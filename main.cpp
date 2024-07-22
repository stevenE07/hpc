#include "easylogging++.h"
#include <iostream>
#include "map"
#include "vector"
#include "include/Calle.h"
#include <filesystem>
#include "include/CargarGrafo.h"
#include "include/Grafo.h"
#include <omp.h>
#include <random>
#include "chrono"
#include <mpi.h>
#include <numeric>
#include "include/Utils.h"
#include <queue>
#include <stdio.h>

#define TAG_CANTIDAD_SEGMENTOS 0
#define TAG_SEGMENTOS 1

#define TAG_SOLICITUDES 2
#define TAG_NOTIFICACIONES 3


INITIALIZE_EASYLOGGINGPP

#include "json.hpp"
#define TIEMPO_EPOCA_MS 100 //En ms

using Clock = std::chrono::steady_clock;
using chrono::time_point;
using chrono::duration_cast;
using chrono::milliseconds;
using chrono::seconds;


using namespace std;


// ----------- Tipos MPI
MPI_Datatype MPI_SegmentoTrayectoVehculoEnBarrio;
MPI_Datatype MPI_SolicitudTranspaso;


// ----------- Variables Globales

int my_rank, size_mpi;
int numero_vehiculos_en_curso_global = 200; //Esto se deberia leer por parametro, se actualiza en cada epoca

int numero_vehiculos_en_curso_en_el_nodo = 0; //Se calcula al generar los vehiculos

map<int, Vehiculo*> mapa_mis_vehiculos;

Grafo* grafoMapa;
map<long, Barrio*> mapa_mis_barios;
vector<long> mis_barrios;
vector<Calle*> todas_calles;

map<long, int> asignacion_barrios;

set<int> nodos_mpi_vecinos;

map<pair<int, long>, queue<SegmentoTrayectoVehculoEnBarrio>> segmentos_a_recorrer_por_barrio_por_vehiculo;

vector<SolicitudTranspaso> solicitudes_transpaso_entre_nodos_mpi;
vector<NotificacionTranspaso> notificaciones_transpaso_entre_nodos_mpi;


// ----------- Funciones auxiliares

void calcular_nodos_mpi_vecinos(){
    nodos_mpi_vecinos.clear();
    for (auto bb: mapa_mis_barios){
        for(auto b_vecino : bb.second->getBarriosVecinos()){
            int id_nodo_encargado = asignacion_barrios[b_vecino];
            if(id_nodo_encargado != my_rank){
                nodos_mpi_vecinos.insert(id_nodo_encargado);
            }
        }
    }

}

void create_mpi_types() {

    // ----- Creacion de MPI_SegmentoTrayectoVehculoEnBarrio
    const int nitems_segmento=3;
    int blocklengths_segmento[3] = {1, 3, 1};
    MPI_Datatype types_segmento[3] = {MPI_INT, MPI_LONG,MPI_BYTE};
    MPI_Aint     offsets_segmento[3];

    offsets_segmento[0] = offsetof(SegmentoTrayectoVehculoEnBarrio, id_vehiculo);
    offsets_segmento[1] = offsetof(SegmentoTrayectoVehculoEnBarrio, id_inicio);
    offsets_segmento[2] = offsetof(SegmentoTrayectoVehculoEnBarrio, is_segmento_final);

    MPI_Type_create_struct(nitems_segmento, blocklengths_segmento, offsets_segmento, types_segmento, &MPI_SegmentoTrayectoVehculoEnBarrio);
    MPI_Type_commit(&MPI_SegmentoTrayectoVehculoEnBarrio);


    const int nitems_solicitud=2;
    int blocklengths_solicitud[2] = {1, 2};
    MPI_Datatype types_solicitud[2] = {MPI_INT, MPI_LONG};
    MPI_Aint     offsets_solicitud[2];

    offsets_solicitud[0] = offsetof(SolicitudTranspaso, id_vehiculo);
    offsets_solicitud[1] = offsetof(SolicitudTranspaso, id_nodo_inicial_calle_anterior);

    MPI_Type_create_struct(nitems_solicitud, blocklengths_solicitud, offsets_solicitud, types_solicitud, &MPI_SolicitudTranspaso);
    MPI_Type_commit(&MPI_SolicitudTranspaso);

}

void crear_vehiculos_de_otros_nodos( vector<SolicitudTranspaso> & solicitudesRecividas){
    for(SolicitudTranspaso solicitud: solicitudesRecividas){
        pair<int, long> claveSegmento = make_pair(solicitud.id_vehiculo, solicitud.id_barrio);
        SegmentoTrayectoVehculoEnBarrio sigSegmento = (segmentos_a_recorrer_por_barrio_por_vehiculo)[claveSegmento].front();

        #pragma omp critical
        segmentos_a_recorrer_por_barrio_por_vehiculo[claveSegmento].pop();

        auto caminoSigBarrio = grafoMapa->computarCaminoMasCorto(sigSegmento.id_inicio, sigSegmento.id_fin); //ToDo mejorar que solo busque en el barrio

        auto vehiculoIngresado = new Vehiculo(solicitud.id_vehiculo);
        mapa_mis_vehiculos[vehiculoIngresado->getId()] =  vehiculoIngresado;

        vehiculoIngresado->setRuta(caminoSigBarrio, sigSegmento.is_segmento_final);
        vehiculoIngresado->set_indice_calle_recorrida(0);
        vehiculoIngresado->setEsperandoTrasladoEntreCalles(false);

        long idBarrioSiguiente = grafoMapa->obtenerNodo(caminoSigBarrio[0])->getSeccion();


        string idSigCalle = Calle::getIdCalle(caminoSigBarrio[0], caminoSigBarrio[1]);
        Calle *sigCalle = mapa_mis_barios[idBarrioSiguiente]->obtenerCalle(idSigCalle);

        sigCalle->insertarSolicitudTranspaso(solicitud.id_nodo_inicial_calle_anterior, caminoSigBarrio[0], vehiculoIngresado);

    }
}

void procesar_notificaciones_recividas (vector<int> & id_vehculos_notificados){
    for( auto idVehiculo: id_vehculos_notificados){
        mapa_mis_vehiculos[idVehiculo]->getCalleactual()->notificarTranspasoCompleto(idVehiculo, true);
        mapa_mis_vehiculos.erase(idVehiculo);
    }
}

void intercambiar_solicitudes(vector<SolicitudTranspaso>* ptr_solicitudes){

    map<int, vector<SolicitudTranspaso>> solicitudes_por_nodos;

    int total_solicitudes = 0;

    auto * total_solicitudes_por_nodo = new int[size_mpi];

    for(int i = 0; i < size_mpi; i++){
        total_solicitudes_por_nodo[i] = 0;
    }

    for(auto solicitud: solicitudes_transpaso_entre_nodos_mpi){
        long id_barrio = solicitud.id_barrio;
        int nodo_encargado = asignacion_barrios[id_barrio];

        total_solicitudes_por_nodo[nodo_encargado]++;
        total_solicitudes++;

        solicitudes_por_nodos[nodo_encargado].push_back(solicitud);
    }

    solicitudes_transpaso_entre_nodos_mpi.clear();

    //auto requests = new MPI_Request[nodos_mpi_vecinos.size()];

    /*int cont = 0;
    for(auto nodo_vecino: nodos_mpi_vecinos){

        MPI_Isend(&total_solicitudes_por_nodo[nodo_vecino], 1, MPI_INT, nodo_vecino, 1, MPI_COMM_WORLD, &requests[cont]);
        cont++;
    }*/

    int size_buffer = total_solicitudes * sizeof(SolicitudTranspaso) + MPI_BSEND_OVERHEAD;
    auto bufferEnvioSolicitudes = (char*)malloc( size_buffer);

    MPI_Buffer_attach(bufferEnvioSolicitudes, size_buffer);

    for(auto nodo_vecino: nodos_mpi_vecinos){
        int numeroSolicitudes = total_solicitudes_por_nodo[nodo_vecino];
        auto buff = new SolicitudTranspaso[numeroSolicitudes];

        int cont = 0;
        for(auto solicitud: solicitudes_por_nodos[nodo_vecino]){
            buff[cont] = solicitud;
            cont++;
        }

        MPI_Bsend(buff, numeroSolicitudes, MPI_SolicitudTranspaso, nodo_vecino, TAG_SOLICITUDES, MPI_COMM_WORLD);

        delete[] buff;
    }

    /*
    int max_numero_solicitudes_por_nodo = 0;
    for(auto nodo_vecino: nodos_mpi_vecinos) {
        int numero_solcitud;
        MPI_Recv(&numero_solcitud, 1, MPI_INT, MPI_ANY_SOURCE, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        max_numero_solicitudes_por_nodo = max(max_numero_solicitudes_por_nodo, numero_solcitud);
    }
    */

    int max_cantidad = nodos_mpi_vecinos.size() * 5000;
    auto buffResepcion = new SolicitudTranspaso[max_cantidad]; //Todo Mejorar estimador

    for(auto nodo_vecino: nodos_mpi_vecinos) {
        int numero_solcitud;

        MPI_Status status;
        MPI_Recv(buffResepcion, max_cantidad, MPI_SolicitudTranspaso, MPI_ANY_SOURCE, TAG_SOLICITUDES, MPI_COMM_WORLD, &status);

        int cantidad_solicitudes_recividas;
        MPI_Get_count(&status, MPI_SolicitudTranspaso, &cantidad_solicitudes_recividas);

        if(cantidad_solicitudes_recividas > 0){
            for(int i = 0; i < cantidad_solicitudes_recividas; i++){
                (*ptr_solicitudes).push_back(buffResepcion[i]);
            }

            //exit(1);
        }
    }

    delete [] buffResepcion;

    //MPI_Waitall(requests, nodos_mpi_vecinos.size());  //ToDO ¿Necesario?

    delete [] total_solicitudes_por_nodo;

    MPI_Buffer_detach(bufferEnvioSolicitudes, &size_buffer);
    delete[] bufferEnvioSolicitudes;
}

void intercambiar_notificaciones(vector<int>* ptr_notificaciones){
    map<int, vector<NotificacionTranspaso>> notificacion_por_nodos;

    int total_notificaciones = 0;

    auto * total_notificaciones_por_nodo = new int[size_mpi];

    for(int i = 0; i < size_mpi; i++){
        total_notificaciones_por_nodo[i] = 0;
    }

    for(auto notificacion: notificaciones_transpaso_entre_nodos_mpi){
        long id_barrio = notificacion.id_barrio;
        int nodo_encargado = asignacion_barrios[id_barrio];

        total_notificaciones_por_nodo[nodo_encargado]++;
        total_notificaciones++;

        notificacion_por_nodos[nodo_encargado].push_back(notificacion);
    }

    notificaciones_transpaso_entre_nodos_mpi.clear();


    auto requests = new MPI_Request[nodos_mpi_vecinos.size()];

    auto bufferEnvioNotificaciones = new int[total_notificaciones];

    int inicioEnvioBuffer = 0;
    int cont_request = 0;
    for(auto nodo_vecino: nodos_mpi_vecinos){

        int cont = inicioEnvioBuffer;
        for(auto notificacion: notificacion_por_nodos[nodo_vecino]){
            bufferEnvioNotificaciones[cont] = notificacion.id_vehiculo;
            cont++;
        }

        MPI_Isend(&bufferEnvioNotificaciones[inicioEnvioBuffer], (int)notificacion_por_nodos[nodo_vecino].size(), MPI_INT, nodo_vecino, TAG_NOTIFICACIONES, MPI_COMM_WORLD, &requests[cont_request]);

        inicioEnvioBuffer += (int)notificacion_por_nodos[nodo_vecino].size();
        cont_request ++;
    }

    int max_cantidad = nodos_mpi_vecinos.size() * 5000;
    auto buffResepcion = new int[max_cantidad]; //Todo Mejorar estimador

    for(auto nodo_vecino: nodos_mpi_vecinos) {
        int numero_solcitud;

        MPI_Status status;
        MPI_Recv(buffResepcion, max_cantidad, MPI_INT, MPI_ANY_SOURCE, TAG_NOTIFICACIONES, MPI_COMM_WORLD, &status);

        int cantidad_notificaciones_recividas;
        MPI_Get_count(&status, MPI_INT, &cantidad_notificaciones_recividas);

        if(cantidad_notificaciones_recividas > 0){
            for(int i = 0; i < cantidad_notificaciones_recividas; i++){
                (*ptr_notificaciones).push_back(buffResepcion[i]);
            }
        }
    }

    delete [] buffResepcion;

    for(int i = 0; i < nodos_mpi_vecinos.size(); i++){
        MPI_Wait(&requests[i], MPI_STATUS_IGNORE);
    }

    delete [] bufferEnvioNotificaciones;
}


time_point<Clock> inicioTiempoEp;

void ejecutar_epoca(int numero_epoca){

    //#pragma omp parallel for  //ToDo descomentar cuando se hagan las pruebas en la fing
    for (int i = 0; i < todas_calles.size(); ++i) {
        auto it = todas_calles[i];
        it->ejecutarEpoca(TIEMPO_EPOCA_MS); // Ejecutar la época para la calle
        //if (numero_epoca % 10 == 0) {
            //#pragma omp critical
            //it->mostrarEstado(); // Mostrar el estado cada 10 épocas
        //}
    }



    numero_vehiculos_en_curso_en_el_nodo -= (int)solicitudes_transpaso_entre_nodos_mpi.size();

    vector<SolicitudTranspaso> solicitudesRecividas;
    intercambiar_solicitudes(&solicitudesRecividas);

    numero_vehiculos_en_curso_en_el_nodo += (int)solicitudesRecividas.size();

    vector<int> notificacionesRecividas;
    intercambiar_notificaciones(&notificacionesRecividas);

    // ToDo esto estaria bueno que fuera tareas y tambien se hagan en paralelo
    crear_vehiculos_de_otros_nodos(solicitudesRecividas);

    procesar_notificaciones_recividas(notificacionesRecividas);

    MPI_Allreduce(&numero_vehiculos_en_curso_en_el_nodo, &numero_vehiculos_en_curso_global, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);



    if(my_rank == 0){
        if((numero_epoca + 1) % 500 == 1){
            milliseconds milisecondsEp = duration_cast<milliseconds>(Clock::now() - inicioTiempoEp);
            cout << " ========  Epoca  " << numero_epoca + 1 << " | Tiempo: " << ( (float)milisecondsEp.count() / 1000.f) / 500.f << " ms | numero vehiculos pendientes " << numero_vehiculos_en_curso_global << " ==========" << endl;
            inicioTiempoEp = Clock::now();
        }
    }


    LOG(INFO) << " ========  Epoca  "<< numero_epoca << " ==========";

}

void initConfig(){
    omp_set_num_threads(6);
    // ---- Configuraciones

    el::Configurations defaultConf;
    defaultConf.setToDefault();
    // Values are always std::string
    defaultConf.set(el::Level::Info,
                    el::ConfigurationType::Format, "%datetime %level %msg");
    std::string loogingFile =  PROJECT_BASE_DIR + std::string("/logs/logs.log");
    std::string configFilePath = PROJECT_BASE_DIR + std::string("/configuraciones/logging.ini");

    el::Configurations conf(configFilePath);
    conf.setGlobally(el::ConfigurationType::Filename, loogingFile);
    el::Loggers::reconfigureLogger("default", conf);

}

void initMpi(int argc, char* argv[]){
    MPI_Init(&argc, &argv);
    create_mpi_types();
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size_mpi);

}

int main(int argc, char* argv[]) {
    std::map<int, int> barrios_con_cantidades;
    std::map<int, double> probabilidad_por_barrio;
    std::map<int, int> asignaciones;
    bool calculo_por_distribucion_cantidad_barrio = false;

    std::string dir = PROJECT_BASE_DIR + std::string("/datos/cantidad_personas_por_barrio_montevideo.csv");
    leerCSVbarrioCantidades( dir ,barrios_con_cantidades);

    calcularProbabilidad(barrios_con_cantidades, probabilidad_por_barrio);

    asignarCantidades(numero_vehiculos_en_curso_global, probabilidad_por_barrio, asignaciones);

    initConfig();
    initMpi(argc, argv);
    std::mt19937 rng(2024); // Semilia random

    // ---- Funciones de notificacion


    auto notificarFinalizacion = std::function<void()>{};
    notificarFinalizacion = [&] () -> void {numero_vehiculos_en_curso_en_el_nodo--;};


    auto ingresarSolicitudTranspaso = std::function<void(SolicitudTranspaso&)>{};
    ingresarSolicitudTranspaso = [&] (SolicitudTranspaso& solicitud) -> void {solicitudes_transpaso_entre_nodos_mpi.push_back(solicitud);};

    auto ingresarNotificacionTranspaso = std::function<void(NotificacionTranspaso &)>{};
    ingresarNotificacionTranspaso = [&] (NotificacionTranspaso & notificacion) -> void {notificaciones_transpaso_entre_nodos_mpi.push_back(notificacion);};



    CargarGrafo loadData = CargarGrafo(PROJECT_BASE_DIR + std::string("/datos/montevideo_por_barrios.json"));
    vector<pair<long, basic_string<char>>> barrios = loadData.obtenerBarrios();




    // ----- Funcion de asignacion de barrios a nodos MPI //ToDO cambiar
    for(int i = 0; i < barrios.size(); i++){
        asignacion_barrios[barrios[i].first] = i % size_mpi;
        if(my_rank == i % size_mpi){
            mis_barrios.push_back(barrios[i].first);
        }
    }

    grafoMapa = new Grafo();
    loadData.FormarGrafo(grafoMapa,
                         mapa_mis_barios,
                         notificarFinalizacion,
                         ingresarSolicitudTranspaso,
                         ingresarNotificacionTranspaso,
                         asignacion_barrios,
                         &segmentos_a_recorrer_por_barrio_por_vehiculo,my_rank);

    calcular_nodos_mpi_vecinos(); //Calculo cuales son mis nodos MPI vecinos

    //Formar un unico arreglo de Calles para todos los barrios del
    for(const auto& id_bario_y_barrio: mapa_mis_barios){
        id_bario_y_barrio.second->addCalles(todas_calles);
    }

    cout << "Nodo " << my_rank << " tiene " << todas_calles.size() << " calles" << endl;


    map<int,long,long> nodos_inicio_final_vehiculo;






    time_point<Clock> inicioTiempoDJ = Clock::now();
    int numeroVehiculosFallo = 0;



    if (my_rank == 0){ //ToDo sacar la limitante que solo el nodo 0 genere los vehiculos

        // --- Genero el inicio y final de cada nodo

        numero_vehiculos_en_curso_en_el_nodo = numero_vehiculos_en_curso_global;

        long* nodo_inicial = new long[numero_vehiculos_en_curso_en_el_nodo];
        long* nodo_final   = new long[numero_vehiculos_en_curso_en_el_nodo];



        if(calculo_por_distribucion_cantidad_barrio) {
            int contador = 0;
            for(auto cantidad_por_barrio : barrios_con_cantidades) {
                for(int i = 0; i < cantidad_por_barrio.second; i++) {
                    long id_barrio = cantidad_por_barrio.first;
                    nodo_inicial[contador] = grafoMapa->idNodoAletorio(rng, id_barrio);
                    nodo_final[contador]   = grafoMapa->idNodoAletorio(rng);
                    contador++;
                }
            }
        }else {
            std::uniform_int_distribution<int> dist(0, (int)mis_barrios.size()-1);
            for(int i = 0 ; i < numero_vehiculos_en_curso_en_el_nodo; i++) {
                long id_barrio = mis_barrios[dist(rng)];
                nodo_inicial[i] = grafoMapa->idNodoAletorio(rng, id_barrio);
                nodo_final[i]   = grafoMapa->idNodoAletorio(rng);
            }
        }

        //   ----  CALCULAR CAMINOS

        cout << "Calculando caminos" << endl;

        //Dado un barrio tiene un vector que contiene datos del tipo (id_vehiculo, nodo_inicial, nodo_final)
        map<long, vector<SegmentoTrayectoVehculoEnBarrio>> nodos_a_enviar_mpi_por_barrio;

        //PARA CADA VEHICULO DEFINIDO CALCULAMOS SU RUTA.
        #pragma omp parallel for schedule(dynamic)
        for(int id_vehiculo = 0 ; id_vehiculo < numero_vehiculos_en_curso_en_el_nodo; id_vehiculo++) {

            int thread_id = omp_get_thread_num();
            long src = nodo_inicial[id_vehiculo];
            long dst = nodo_final[id_vehiculo];

            auto camino = grafoMapa->computarCaminoMasCorto(src,dst);


            if(camino.empty()){
                #pragma omp atomic
                numeroVehiculosFallo++;
            } else {

                long barrio_actual = grafoMapa->obtenerNodo(src)->getSeccion();
                auto nodo_inicial_barrio = camino.front();

                int size_camino = camino.size();

                /*
                for (auto c : camino){
                    long id_barrio = grafoMapa->obtenerNodo(c)->getSeccion();
                    cout << id_barrio << " --- " << c << endl;
                }
                 */

                for (size_t j = 0; j < size_camino; ++j) {
                    int barrio = grafoMapa->obtenerNodo(camino[j])->getSeccion();
                    if(barrio != barrio_actual || j == size_camino - 1) {

                        SegmentoTrayectoVehculoEnBarrio segmento{};
                        segmento.id_vehiculo = id_vehiculo;
                        segmento.id_inicio = nodo_inicial_barrio;
                        segmento.id_fin = camino[j];
                        segmento.id_barrio = barrio_actual;
                        segmento.is_segmento_final = (j == size_camino - 1) ? 1 : 0;


                        if (std::find(mis_barrios.begin(), mis_barrios.end(), barrio_actual) == mis_barrios.end() ){ //El barrio no es parte de mi barrio
                            #pragma omp critical
                            nodos_a_enviar_mpi_por_barrio[barrio_actual].push_back(segmento);
                        } else {
                            // Si el segmento es para el nodo que calcula, se lo guarda para si mismo
                            pair<int, long> clave;
                            clave.first = id_vehiculo;
                            clave.second = barrio_actual;

                            #pragma omp critical
                            segmentos_a_recorrer_por_barrio_por_vehiculo[clave].push(segmento);
                        }


                        barrio_actual = barrio;
                        nodo_inicial_barrio = camino[j];
                    }
                }

                auto v = new Vehiculo(id_vehiculo);

                pair<int, long> claveBarioVehiculo = make_pair(v->getId(), grafoMapa->obtenerNodo(src)->getSeccion());

                SegmentoTrayectoVehculoEnBarrio primerSegmento = segmentos_a_recorrer_por_barrio_por_vehiculo[claveBarioVehiculo].front();

                #pragma omp critical
                segmentos_a_recorrer_por_barrio_por_vehiculo[claveBarioVehiculo].pop();


                auto caminoPrimerBarrio = grafoMapa->computarCaminoMasCorto(primerSegmento.id_inicio, primerSegmento.id_fin); //ToDo mejorar que solo busque en el barrio

                v->setRuta(caminoPrimerBarrio, primerSegmento.is_segmento_final);

                #pragma omp critical
                mapa_mis_vehiculos[v->getId()] = v;

                long id_camino_primer_nodo = caminoPrimerBarrio[0];
                long id_camino_segundo_nodo = caminoPrimerBarrio[1];

                string id_calle = Calle::getIdCalle(id_camino_primer_nodo, id_camino_segundo_nodo);
                Nodo* nodo_inicial_r = grafoMapa->obtenerNodo(id_camino_primer_nodo);
                Calle * calle = mapa_mis_barios[nodo_inicial_r->getSeccion()]->obtenerCalle(id_calle);
                calle->insertarSolicitudTranspaso(-1, -1, v);

            }
        }

        numero_vehiculos_en_curso_en_el_nodo -= numeroVehiculosFallo;

        delete[] nodo_inicial;
        delete[] nodo_final;

        //enviar para cada nodo mpi la informacion que le corresponde.
        for(int rank_nodo_a_enviar = 1; rank_nodo_a_enviar < size_mpi; rank_nodo_a_enviar ++) {
            int cantidad_mensajes_a_enviar = 0;

            for (const auto &par: asignacion_barrios)
                if (par.second == rank_nodo_a_enviar && !nodos_a_enviar_mpi_por_barrio[par.first].empty())
                    cantidad_mensajes_a_enviar += nodos_a_enviar_mpi_por_barrio[par.first].size();



            //computo por cada barrio los vehiculos que le corresponden y lo agrego a la estructura de data_vehiculo_a_enviar.

            auto data_vehiculo_a_enviar = new SegmentoTrayectoVehculoEnBarrio[cantidad_mensajes_a_enviar];
            int cont = 0;

            for (auto bb: asignacion_barrios) {
                long idBarrio = bb.first;
                if (bb.second == rank_nodo_a_enviar && !nodos_a_enviar_mpi_por_barrio[idBarrio].empty()) {
                    for (auto &m: nodos_a_enviar_mpi_por_barrio[idBarrio]) {
                        data_vehiculo_a_enviar[cont] =  m;
                        cont++;
                    }
                }
            }


            MPI_Send(&cantidad_mensajes_a_enviar, 1, MPI_INT, rank_nodo_a_enviar, TAG_CANTIDAD_SEGMENTOS, MPI_COMM_WORLD);
            MPI_Send(data_vehiculo_a_enviar, cantidad_mensajes_a_enviar, MPI_SegmentoTrayectoVehculoEnBarrio, rank_nodo_a_enviar, TAG_SEGMENTOS,MPI_COMM_WORLD);

            printf("Envio de segmentos de nodo %d a nodo %d COMPLETADO\n", my_rank, rank_nodo_a_enviar);

            delete[] data_vehiculo_a_enviar;
        }
    } else {
        MPI_Status status;
        int cantidad_vehiculos_barrios;

        MPI_Recv(&cantidad_vehiculos_barrios, 1, MPI_INT, 0, TAG_CANTIDAD_SEGMENTOS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        auto inicio_fin_vehculos_nodo = new SegmentoTrayectoVehculoEnBarrio[cantidad_vehiculos_barrios];

        MPI_Recv(inicio_fin_vehculos_nodo,cantidad_vehiculos_barrios,MPI_SegmentoTrayectoVehculoEnBarrio,0,TAG_SEGMENTOS,MPI_COMM_WORLD,MPI_STATUS_IGNORE);


        for(int i = 0; i < cantidad_vehiculos_barrios; i++){
            pair<int,long> clave = make_pair(inicio_fin_vehculos_nodo[i].id_vehiculo, inicio_fin_vehculos_nodo[i].id_barrio);
            segmentos_a_recorrer_por_barrio_por_vehiculo[clave].push(inicio_fin_vehculos_nodo[i]);
        }

    }

    milliseconds milisecondsDj = duration_cast<milliseconds>(Clock::now() - inicioTiempoDJ);
    printf("----- Tiempo transcurido calculo y distribuccion caminos = %.2f seg \n", (float)milisecondsDj.count() / 1000.f);



    time_point<Clock> inicioTiempo = Clock::now();

    inicioTiempoEp = Clock::now();

    int contador_numero_epoca = 1;
    while(numero_vehiculos_en_curso_global > 0){
        ejecutar_epoca(contador_numero_epoca);
        contador_numero_epoca++;
    }

    milliseconds miliseconds = duration_cast<milliseconds>(Clock::now() - inicioTiempo);
    if(my_rank == 0){
        printf("----- Tiempo transcurido simulacion = %.2f seg \n", (float)miliseconds.count() / 1000.f);
    }


    // Limpieza

    for (auto barrios : mapa_mis_barios){
        delete barrios.second;
    }

    for (auto v: mapa_mis_vehiculos){
        delete v.second;
    }


    MPI_Finalize();


    return 0;

}

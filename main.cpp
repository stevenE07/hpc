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

#include <stdio.h>


INITIALIZE_EASYLOGGINGPP

#include "json.hpp"
#define TIEMPO_EPOCA_MS 10 //En ms

using Clock = std::chrono::steady_clock;
using chrono::time_point;
using chrono::duration_cast;
using chrono::milliseconds;
using chrono::seconds;


using namespace std;



// ----------- Tipos MPI
MPI_Datatype MPI_SegmentoTrayectoVehculoEnBarrio;
MPI_Datatype MPI_SolicitudTranspaso;
MPI_Datatype MPI_NotificacionTranspaso;


// ----------- Variables Globales
map<long, Barrio*> mapa_mis_barios;
vector<long> mis_barrios;
vector<Calle*> todas_calles;

map<pair<int, long>, SegmentoTrayectoVehculoEnBarrio> segmentos_a_recorrer_por_barrio_por_vehiculo;

vector<SolicitudTranspaso> solicitudes_transpaso_entre_nodos_mpi;


void create_mpi_types() {

    // ----- Creacion de MPI_SegmentoTrayectoVehculoEnBarrio
    const int nitems_segmento=2;
    int blocklengths_segmento[2] = {1, 3};
    MPI_Datatype types_segmento[2] = {MPI_INT, MPI_LONG};
    MPI_Aint     offsets_segmento[2];

    offsets_segmento[0] = offsetof(SegmentoTrayectoVehculoEnBarrio, id_vehiculo);
    offsets_segmento[1] = offsetof(SegmentoTrayectoVehculoEnBarrio, id_inicio);

    MPI_Type_create_struct(nitems_segmento, blocklengths_segmento, offsets_segmento, types_segmento, &MPI_SegmentoTrayectoVehculoEnBarrio);
    MPI_Type_commit(&MPI_SegmentoTrayectoVehculoEnBarrio);


    const int nitems_solicitud=2;
    int blocklengths_solicitud[2] = {1, 3};
    MPI_Datatype types_solicitud[2] = {MPI_INT, MPI_LONG};
    MPI_Aint     offsets_solicitud[2];

    offsets_solicitud[0] = offsetof(SolicitudTranspaso, id_vehiculo);
    offsets_solicitud[1] = offsetof(SolicitudTranspaso, id_nodo_inicial_calle_anterior);

    MPI_Type_create_struct(nitems_solicitud, blocklengths_solicitud, offsets_solicitud, types_solicitud, &MPI_SolicitudTranspaso);
    MPI_Type_commit(&MPI_SolicitudTranspaso);

}

void intercambiar_vehiculos_entre_nodos(){

    map< //Obtener lista con los nodos con los que me tengo que comunir y enviarle su parte del array solicitudes_transpaso_entre_nodos_mpi
        // son la 4 am ya no me funciona mas el cerebo, sigo mañana...

}

void ejecutar_epoca(int numero_epoca, long num_vehioculo){

    if(numero_epoca % 500 == 1){
        cout << " ========  Epoca  "<< numero_epoca << " | "<< num_vehioculo << " ==========" << endl;
    }
    LOG(INFO) << " ========  Epoca  "<< numero_epoca << " ==========";

    #pragma omp parallel for
    for (int i = 0; i < todas_calles.size(); ++i) {
        auto it = todas_calles[i];
        it->ejecutarEpoca(TIEMPO_EPOCA_MS); // Ejecutar la época para la calle
        //if (numero_epoca % 10 == 0) {
            //#pragma omp critical
            //it->mostrarEstado(); // Mostrar el estado cada 10 épocas
        //}
    }

    intercambiar_vehiculos_entre_nodos();

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

void initMpi(int argc, char* argv[], int & rank, int & size){
    MPI_Init(&argc, &argv);
    create_mpi_types();
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

}

int main(int argc, char* argv[]) {

    initConfig();

    int rank, size;
    initMpi(argc, argv, rank, size);


    std::mt19937 rng(2024); // Semilia random



    // ---- Funciones de notificacion

    long numeroVehiculosPendientes = 1;

    auto notificarFinalizacion = std::function<void()>{};
    notificarFinalizacion = [&] () -> void {numeroVehiculosPendientes--;};


    auto ingresarSolicitudTranspaso = std::function<void(SolicitudTranspaso&)>{};
    ingresarSolicitudTranspaso = [&] (SolicitudTranspaso& solicitud) -> void {solicitudes_transpaso_entre_nodos_mpi.push_back(solicitud);};



    CargarGrafo loadData = CargarGrafo(PROJECT_BASE_DIR + std::string("/datos/montevideo_por_barrios.json"));
    vector<pair<long, basic_string<char>>> barrios = loadData.obtenerBarrios();


    map<long, int> asignacion_barrios;

    // ----- Funcion de asignacion de barrios a nodos MPI //ToDO cambiar
    for(int i = 0; i < barrios.size(); i++){
        asignacion_barrios[barrios[i].first] = i % size;
        if(rank == i % size){
            mis_barrios.push_back(barrios[i].first);
        }
    }



    auto grafoMapa = new Grafo();
    loadData.FormarGrafo(grafoMapa, mapa_mis_barios,
                         notificarFinalizacion, ingresarSolicitudTranspaso,
                         asignacion_barrios, rank);


    //Formar un unico arreglo de Calles para todos los barrios del
    for(const auto& id_bario_y_barrio: mapa_mis_barios){
        id_bario_y_barrio.second->addCalles(todas_calles);
    }



    map<int,long,long> nodos_inicio_final_vehiculo;

    vector<Vehiculo*> todos_vehiculos;




    time_point<Clock> inicioTiempoDJ = Clock::now();
    int numeroVehiculosFallo = 0;



    if (rank == 0){ //ToDo sacar la limitante que solo el nodo 0 genere los vehiculos

        // --- Genero el inicio y final de cada nodo

        long* nodo_inicial = new long[numeroVehiculosPendientes];
        long* nodo_final   = new long[numeroVehiculosPendientes];

        std::uniform_int_distribution<int> dist(0, (int)mis_barrios.size()-1);
        for(int i = 0 ; i < numeroVehiculosPendientes; i++) {

            //Genero unicamente vehiculos cuyas ubicaciones comiencen en el nodo

            long id_barrio = mis_barrios[dist(rng)];
            nodo_inicial[i] = grafoMapa->idNodoAletorio(rng, id_barrio);
            nodo_final[i]   = grafoMapa->idNodoAletorio(rng);
        }

        //   ----  CALCULAR CAMINOS

        cout << "Calculando caminos" << endl;

        //Dado un barrio tiene un vector que contiene datos del tipo (id_vehiculo, nodo_inicial, nodo_final)
        map<long, vector<SegmentoTrayectoVehculoEnBarrio>> nodos_a_enviar_mpi_por_barrio;

        //PARA CADA VEHICULO DEFINIDO CALCULAMOS SU RUTA.
        #pragma omp parallel for schedule(dynamic)
        for(int id_vehiculo = 0 ; id_vehiculo < numeroVehiculosPendientes; id_vehiculo++) {

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

                for (size_t j = 0; j < camino.size(); ++j) {
                    int barrio = grafoMapa->obtenerNodo(camino[j])->getSeccion();
                    if(barrio != barrio_actual) {

                        SegmentoTrayectoVehculoEnBarrio segmento{};
                        segmento.id_vehiculo = id_vehiculo;
                        segmento.id_inicio = nodo_inicial_barrio;
                        segmento.id_fin = camino[j];
                        segmento.id_barrio = barrio_actual;

                        if (std::find(mis_barrios.begin(), mis_barrios.end(), barrio_actual) == mis_barrios.end() ){ //El barrio no es parte de mi barrio
                            nodos_a_enviar_mpi_por_barrio[barrio_actual].push_back(segmento);
                        } else {
                            // Si el segmento es para el nodo que calcula, se lo guarda para si mismo
                            pair<int, long> clave;
                            clave.first = id_vehiculo;
                            clave.second = barrio_actual;
                            segmentos_a_recorrer_por_barrio_por_vehiculo[clave] = segmento;
                        }


                        barrio_actual = barrio;
                        nodo_inicial_barrio = camino[j];
                    }
                }

                auto v = new Vehiculo(id_vehiculo, 0, 45);
                v->setRuta(camino);
                todos_vehiculos.push_back(v);
                long id_camino_primer_nodo = camino[0];
                long id_camino_segundo_nodo = camino[1];

                cout << id_camino_primer_nodo << " " << id_camino_segundo_nodo << endl;

                string id_calle = Calle::getIdCalle(id_camino_primer_nodo, id_camino_segundo_nodo);
                Nodo* nodo_inicial_r = grafoMapa->obtenerNodo(id_camino_primer_nodo);
                Calle * calle = mapa_mis_barios[nodo_inicial_r->getSeccion()]->obtenerCalle(id_calle);
                calle->insertarSolicitudTranspaso(nullptr, v);

            }
        }



        delete[] nodo_inicial;
        delete[] nodo_final;

        //enviar para cada nodo mpi la informacion que le corresponde.
        for(int rank_nodo_a_enviar = 1; rank_nodo_a_enviar < size; rank_nodo_a_enviar ++) {
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

            printf("Cantidad segmentos a enviar nodo %d: %d\n", rank_nodo_a_enviar, cantidad_mensajes_a_enviar);
            MPI_Send(&cantidad_mensajes_a_enviar, 1, MPI_INT, rank_nodo_a_enviar, 0, MPI_COMM_WORLD);

            printf("Cantidad segmentos a enviar datos a  nodo %d:\n", rank_nodo_a_enviar);
            MPI_Send(data_vehiculo_a_enviar, cantidad_mensajes_a_enviar, MPI_SegmentoTrayectoVehculoEnBarrio, rank_nodo_a_enviar, 0,
                     MPI_COMM_WORLD);

            printf("Envio de segmentos de nodo %d a nodo %d COMPLETADO\n", rank, rank_nodo_a_enviar);

            delete[] data_vehiculo_a_enviar;
        }
    } else {
        MPI_Status status;
        int cantidad_vehiculos_barrios;

        printf("Nodo %d, esperando numero caminos\n", rank);
        MPI_Recv(&cantidad_vehiculos_barrios, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        printf("Nodo %d, esperando %d segmentos\n", rank, cantidad_vehiculos_barrios);
        auto inicio_fin_vehculos_nodo = new SegmentoTrayectoVehculoEnBarrio[cantidad_vehiculos_barrios];

        MPI_Recv(inicio_fin_vehculos_nodo,cantidad_vehiculos_barrios,MPI_SegmentoTrayectoVehculoEnBarrio,0,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);

        printf("Nodo %d, recibio %d trayectos", rank, cantidad_vehiculos_barrios);

    }

    milliseconds milisecondsDj = duration_cast<milliseconds>(Clock::now() - inicioTiempoDJ);
    printf("----- Tiempo transcurido = %.2f seg \n", (float)milisecondsDj.count() / 1000.f);

    numeroVehiculosPendientes -= numeroVehiculosFallo;






    time_point<Clock> inicioTiempo = Clock::now();

    int contador_numero_epoca = 1;
    while(numeroVehiculosPendientes > 0){
        ejecutar_epoca(contador_numero_epoca, numeroVehiculosPendientes);
        contador_numero_epoca++;
    }

    milliseconds miliseconds = duration_cast<milliseconds>(Clock::now() - inicioTiempo);
    printf("----- Tiempo transcurido = %.2f seg \n", (float)miliseconds.count() / 1000.f);


    // Limpieza

    for (auto barrios : mapa_mis_barios){
        delete barrios.second;
    }

    for (Vehiculo* v: todos_vehiculos){
        delete v;
    }


    MPI_Finalize();


    return 0;

}

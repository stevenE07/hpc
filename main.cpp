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

#define MAX_SOLICITUDES_NOTIFICIACION_RECIVIDAS_POR_NODO_POR_EPOCA 1000


INITIALIZE_EASYLOGGINGPP

#include "json.hpp"
#define TIEMPO_EPOCA_MS 100 //En ms

using Clock = std::chrono::steady_clock;
using chrono::time_point;
using chrono::duration_cast;
using chrono::milliseconds;
using chrono::seconds;


using namespace std;

std::string config_dir = PROJECT_BASE_DIR + std::string("/configuraciones/config.txt");
std::map<std::string, std::string> conf = readConfig(config_dir);

// ----------- Tipos MPI
MPI_Datatype MPI_SegmentoTrayectoVehculoEnBarrio;
MPI_Datatype MPI_SolicitudTranspaso;
MPI_Datatype MPI_AsignacionBarrio;


// ----------- Variables Globales

time_point<Clock> inicioTiempoEp;


int my_rank, size_mpi;

int numero_vehiculos_en_curso_global = stoi(conf["numero_vehiculos_en_curso_global"]); //Esto se deberia leer por parametro, se actualiza en cada epoca
int numero_vehiculos_en_curso_en_el_nodo = 0; //Se calcula al generar los vehiculos

map<int, Vehiculo*> mapa_mis_vehiculos;

Grafo* grafoMapa;
map<long, Barrio*> mapa_mis_barios;
vector<long> mis_barrios;
vector<Calle*> todas_calles;

map<long, int> asignacion_barrios;

vector<int> nodos_mpi_vecinos;

map<pair<int, long>, queue<SegmentoTrayectoVehculoEnBarrio>> segmentos_a_recorrer_por_barrio_por_vehiculo;

vector<SolicitudTranspaso> solicitudes_transpaso_entre_nodos_mpi;
vector<NotificacionTranspaso> notificaciones_transpaso_entre_nodos_mpi;

vector<float> tiempoRecorridoPor10Km;
map<long,vector<float>> tiempoRecorridoPor10KmBarrio;



// --- Buffers de Resepcion y Envio de notificacion y solicitudes

SolicitudTranspaso * bufferEnvioSolicitudesEpocaActual;
int * bufferEnvioNotificacionesEpocaActual;

SolicitudTranspaso ** buffRecepcionSolicitudes;
int * cantidadSolicitudesRecibidas;


int ** buffRecepcionNotificaciones;
int * cantidadNotificacionesRecibidas;

SolicitudTranspaso * allsolicitudesRecividas;
int * allNotificacionesRecividas;
int numAllSolicitudesRecividas;
int numAllNotificacionesRecividas;

//ToDo mover al metodo
MPI_Request* requestsSendSolicitudes;
MPI_Request* requestsSendNotificaciones;

MPI_Request* requestRecvSolicitudes;
MPI_Request* requestRecvNotificaciones;


// ----------- Funciones auxiliares

void calcular_nodos_mpi_vecinos(){
    nodos_mpi_vecinos.clear();

    set<int> nodosVecinosSet;

    for (auto bb: mapa_mis_barios){
        for(auto b_vecino : bb.second->getBarriosVecinos()){
            int id_nodo_encargado = asignacion_barrios[b_vecino];
            if(id_nodo_encargado != my_rank){
                nodosVecinosSet.insert(id_nodo_encargado);
            }
        }
    }

    for(auto v : nodosVecinosSet){
        nodos_mpi_vecinos.push_back(v);
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


    const int nitems_solicitud=3;
    int blocklengths_solicitud[3] = {2, 1, 2};
    MPI_Datatype types_solicitud[3] = {MPI_INT, MPI_FLOAT, MPI_LONG};
    MPI_Aint     offsets_solicitud[3];

    offsets_solicitud[0] = offsetof(SolicitudTranspaso, id_vehiculo);
    offsets_solicitud[1] = offsetof(SolicitudTranspaso, trayectoriaTotal);
    offsets_solicitud[2] = offsetof(SolicitudTranspaso, id_nodo_inicial_calle_anterior);

    MPI_Type_create_struct(nitems_solicitud, blocklengths_solicitud, offsets_solicitud, types_solicitud, &MPI_SolicitudTranspaso);
    MPI_Type_commit(&MPI_SolicitudTranspaso);


    const int nitems_asignacion=2;
    int blocklengths_asignacion[2] = {1, 2};
    MPI_Datatype types_asignacion[2] = {MPI_LONG, MPI_INT};
    MPI_Aint     offsets_asignacion[2];

    offsets_asignacion[0] = offsetof(Asignacion_barrio, id_barrio);
    offsets_asignacion[1] = offsetof(Asignacion_barrio, cantidad_vehiculos_a_generar);

    MPI_Type_create_struct(nitems_asignacion, blocklengths_asignacion, offsets_asignacion, types_asignacion, &MPI_AsignacionBarrio);
    MPI_Type_commit(&MPI_AsignacionBarrio);
}

void procesar_solicitudes_recividas(SolicitudTranspaso* solicitudesRecividas, int & cantidadSolicitudesRecivdas){
    #pragma omp for schedule(dynamic, 1) nowait
    for(int s = 0; s < cantidadSolicitudesRecivdas; s++){
        auto solicitud = solicitudesRecividas[s];
        pair<int, long> claveSegmento = make_pair(solicitud.id_vehiculo, solicitud.id_barrio);

        SegmentoTrayectoVehculoEnBarrio sigSegmento = (segmentos_a_recorrer_por_barrio_por_vehiculo)[claveSegmento].front();

        segmentos_a_recorrer_por_barrio_por_vehiculo[claveSegmento].pop();

        Nodo* nodoInicial = grafoMapa->obtenerNodo(sigSegmento.id_inicio);
        bool rutaPrecargada;
        vector<long> caminoSigBarrio;

        nodoInicial->consultarRutaPreCargada(sigSegmento.id_fin, caminoSigBarrio, rutaPrecargada);
        if(!rutaPrecargada){
            float peso;
            caminoSigBarrio = grafoMapa->computarCaminoMasCorto(sigSegmento.id_inicio, sigSegmento.id_fin, sigSegmento.id_barrio, peso);
            nodoInicial->agregarRutaPreCargada(sigSegmento.id_fin, caminoSigBarrio);
        }

        auto vehiculoIngresado = new Vehiculo(solicitud.id_vehiculo);

        #pragma omp critical(mapa_mis_vehiculos_mutex)
        mapa_mis_vehiculos[vehiculoIngresado->getId()] = vehiculoIngresado;

        vehiculoIngresado->setRuta(caminoSigBarrio, sigSegmento.is_segmento_final);
        vehiculoIngresado->set_indice_calle_recorrida(0);
        vehiculoIngresado->setEpocaInicio(solicitud.epocaInicial);
        vehiculoIngresado->setDistanciaRecorrida(solicitud.trayectoriaTotal);

        long idBarrioSiguiente = grafoMapa->obtenerNodo(caminoSigBarrio[0])->getSeccion();

        string idSigCalle = Calle::getIdCalle(caminoSigBarrio[0], caminoSigBarrio[1]);
        Calle *sigCalle = mapa_mis_barios[idBarrioSiguiente]->obtenerCalle(idSigCalle);

        sigCalle->insertarSolicitudTranspaso(solicitud.id_nodo_inicial_calle_anterior, caminoSigBarrio[0], vehiculoIngresado);

    }
}

void procesar_notificaciones_recividas (int* notificacionesRecividas, int & cantidadNotificaciones){
    for (int s = 0; s < cantidadNotificaciones; s++){
        auto idVehiculo = notificacionesRecividas[s];
        #pragma omp critical(mapa_mis_vehiculos_mutex)
        {
            bool eliminar = mapa_mis_vehiculos[idVehiculo]->getCalleactual()->notificarTranspasoCompleto(idVehiculo, true);
            if(eliminar){
                delete mapa_mis_vehiculos[idVehiculo];
            }
            mapa_mis_vehiculos.erase(idVehiculo);
        }
    }
}



void iniciar_a_recivir_solicitudes(MPI_Request * requestRecv){
    int contador = 0;
    for(auto nodo_vecino: nodos_mpi_vecinos) {
        MPI_Irecv(buffRecepcionSolicitudes[nodo_vecino],
                  MAX_SOLICITUDES_NOTIFICIACION_RECIVIDAS_POR_NODO_POR_EPOCA,
                  MPI_SolicitudTranspaso,
                  nodo_vecino,
                  TAG_SOLICITUDES,
                  MPI_COMM_WORLD,
                  &requestRecv[contador]);
        contador++;

    }
}

void enviar_solicitudes( vector<SolicitudTranspaso>& solicitudes_a_enviar, SolicitudTranspaso* buff, MPI_Request* requestsSolcitudes){

    map<int, vector<SolicitudTranspaso>> solicitudes_por_nodos;

    int total_solicitudes = 0;

    for(auto solicitud: solicitudes_a_enviar){
        long id_barrio = solicitud.id_barrio;
        int nodo_encargado = asignacion_barrios[id_barrio];
        total_solicitudes++;

        solicitudes_por_nodos[nodo_encargado].push_back(solicitud);
    }


    int inicioEnvioBuffer = 0;

    int contador = 0;
    for(auto nodo_vecino: nodos_mpi_vecinos){
        int numeroSolicitudes = (int)solicitudes_por_nodos[nodo_vecino].size();
        int cont = inicioEnvioBuffer;
        for(auto solicitud: solicitudes_por_nodos[nodo_vecino]){
            buff[cont] = solicitud;
            cont++;
        }


        MPI_Isend(&buff[inicioEnvioBuffer],
                  numeroSolicitudes,
                  MPI_SolicitudTranspaso,
                  nodo_vecino,
                  TAG_SOLICITUDES,
                  MPI_COMM_WORLD,
                  &requestsSolcitudes[contador]);

        contador++;
        inicioEnvioBuffer = cont;
    }

    solicitudes_a_enviar.clear();

    //printf("FIN SOLICITUDES DE %d es %d | %d\n", my_rank, inicioEnvioBuffer, total_solicitudes);
}


void iniciar_recibir_notificaciones(MPI_Request * requestRecv){
    int contador = 0;
    for(auto nodo_vecino: nodos_mpi_vecinos) {
        MPI_Irecv(buffRecepcionNotificaciones[nodo_vecino],
                 MAX_SOLICITUDES_NOTIFICIACION_RECIVIDAS_POR_NODO_POR_EPOCA,
                 MPI_INT,
                  nodo_vecino,
                 TAG_NOTIFICACIONES,
                 MPI_COMM_WORLD,
                 &requestRecv[contador]);
        contador++;
    }
}


void leer_solicitud_recibidas(SolicitudTranspaso* todasLasSolicitudesRecibidas, int & totalCantidadSolicitudRecividas){
    for(auto nodo_vecino: nodos_mpi_vecinos) {
        for(int i = 0; i < cantidadSolicitudesRecibidas[nodo_vecino]; i++){
            todasLasSolicitudesRecibidas[totalCantidadSolicitudRecividas] = buffRecepcionSolicitudes[nodo_vecino][i];
            totalCantidadSolicitudRecividas++;
        }

    }
}

void leer_notificaciones_recibidas(int* todasLasNotificacionesRecibidas, int & totalCantidaNotificacionesRecividas){
    for(auto nodo_vecino: nodos_mpi_vecinos) {
        for(int i = 0; i < cantidadNotificacionesRecibidas[nodo_vecino]; i++){
            todasLasNotificacionesRecibidas[totalCantidaNotificacionesRecividas] = buffRecepcionNotificaciones[nodo_vecino][i];
            totalCantidaNotificacionesRecividas++;
        }

    }
}


void enviar_notificaciones(vector<NotificacionTranspaso>& notificaciones_a_enviar, int* buffNotificaciones, MPI_Request* requestsNotificaciones){
    map<int, vector<NotificacionTranspaso>> notificacion_por_nodos;

    int total_notificaciones = 0;

    for(auto notificacion: notificaciones_a_enviar){
        long id_barrio = notificacion.id_barrio;
        int nodo_encargado = asignacion_barrios[id_barrio];
        total_notificaciones++;

        notificacion_por_nodos[nodo_encargado].push_back(notificacion);
    }

    notificaciones_a_enviar.clear();

    int inicioEnvioBuffer = 0;
    int contador = 0;
    for(auto nodo_vecino: nodos_mpi_vecinos){
        int cont = inicioEnvioBuffer;
        for(auto notificacion: notificacion_por_nodos[nodo_vecino]){
            buffNotificaciones[cont] = notificacion.id_vehiculo;
            cont++;
        }

        MPI_Request request;

        MPI_Isend(&buffNotificaciones[inicioEnvioBuffer],
                  (int)notificacion_por_nodos[nodo_vecino].size(),
                  MPI_INT,
                  nodo_vecino,
                  TAG_NOTIFICACIONES,
                  MPI_COMM_WORLD,
                  &requestsNotificaciones[contador]);

        contador++;
        inicioEnvioBuffer = cont;
    }
}

void ejecutar_epoca() {

    vector<SolicitudTranspaso> solicitudes_traspaso_epoca_anterior;
    vector<NotificacionTranspaso> notificaciones_traspaso_epoca_anterior;

    int numero_epoca = 0;

    allsolicitudesRecividas = new SolicitudTranspaso[nodos_mpi_vecinos.size() * MAX_SOLICITUDES_NOTIFICIACION_RECIVIDAS_POR_NODO_POR_EPOCA];
    allNotificacionesRecividas = new int[nodos_mpi_vecinos.size() * MAX_SOLICITUDES_NOTIFICIACION_RECIVIDAS_POR_NODO_POR_EPOCA];

    vector<Nodo*> nodosDelGrafoAsignados;
    for(auto nodo :grafoMapa->obtenerNodos()){
        if(asignacion_barrios[nodo->getSeccion()] == my_rank){
            nodosDelGrafoAsignados.push_back(nodo);
        }
    }



    int numeroDeNodos = (int)nodosDelGrafoAsignados.size();

    if(my_rank == 0){
        cout << "Inicio simulacion" << endl;
    }


    #pragma omp parallel
    {
        do {
            #pragma omp master
            {
                if (my_rank == 0) {
                    if ((numero_epoca + 1) % 500 == 0) {
                        milliseconds milisecondsEp = duration_cast<milliseconds>(Clock::now() - inicioTiempoEp);
                        cout << " ========  Epoca  " << numero_epoca << " | Tiempo: "
                             << ((float) milisecondsEp.count() / 1000.f) / 500.f << " ms | numero vehiculos pendientes "
                             << numero_vehiculos_en_curso_global << " ==========" << endl;
                        inicioTiempoEp = Clock::now();

                    };
                }

                if(numero_epoca > 0){
                    for(int i = 0; i< nodos_mpi_vecinos.size(); i++){
                        MPI_Wait(&requestsSendSolicitudes[i], MPI_STATUS_IGNORE);
                    }
                    for(int i = 0; i< nodos_mpi_vecinos.size(); i++){
                        MPI_Wait(&requestsSendNotificaciones[i], MPI_STATUS_IGNORE);
                    }

                    delete [] requestsSendSolicitudes;
                    delete [] requestsSendNotificaciones;
                    delete [] requestRecvSolicitudes;
                    delete [] requestRecvNotificaciones;
                }

                int numVecinos = (int)nodos_mpi_vecinos.size();
                requestsSendSolicitudes = new MPI_Request[numVecinos];
                requestsSendNotificaciones = new MPI_Request[numVecinos];
                requestRecvSolicitudes = new MPI_Request[numVecinos];
                requestRecvNotificaciones = new MPI_Request[numVecinos];

                bufferEnvioSolicitudesEpocaActual = new SolicitudTranspaso[(int)solicitudes_traspaso_epoca_anterior.size()];
                bufferEnvioNotificacionesEpocaActual = new int[(int)notificaciones_traspaso_epoca_anterior.size()];

                enviar_solicitudes(solicitudes_traspaso_epoca_anterior, bufferEnvioSolicitudesEpocaActual, requestsSendSolicitudes);
                enviar_notificaciones(notificaciones_traspaso_epoca_anterior, bufferEnvioNotificacionesEpocaActual, requestsSendNotificaciones);

                iniciar_a_recivir_solicitudes(requestRecvSolicitudes);
                iniciar_recibir_notificaciones(requestRecvNotificaciones);
            }

            #pragma omp for schedule(dynamic, 150)
            for (int i = 0; i < todas_calles.size(); i++) {
                auto it = todas_calles[i];
                it->ejecutarEpoca(TIEMPO_EPOCA_MS, numero_epoca); // Ejecutar la época para la calle
            }

            #pragma omp barrier

            if((numero_epoca + 1) % 1000 == 0){
                #pragma omp for schedule(dynamic, 50)
                for (int index_nodo = 0; index_nodo < numeroDeNodos; index_nodo++){
                    Nodo* nodoAActualizar = nodosDelGrafoAsignados[index_nodo];

                    long idNodoInicio = nodoAActualizar->getIdExt();
                    long idBarrio = nodoAActualizar->getSeccion();

                    vector<pair<Nodo*, float>> nuevosPesosVecinos;

                    for(auto nodoVecino : nodoAActualizar->getNodosVecinos()){
                        long idNodoFin = nodoVecino.first->getIdExt();

                        string idCalle = Calle::getIdCalle(idNodoInicio, idNodoFin);
                        Calle* calle = mapa_mis_barios[idBarrio]->obtenerCalle(idCalle);

                        float nuevoCosto = calle->obtenerNuevoCostoYLimpiarMedidas();

                        nuevosPesosVecinos.push_back(make_pair(nodoVecino.first, nuevoCosto));
                    }

                    nodoAActualizar->setNodosVecinos(nuevosPesosVecinos);
                    nodoAActualizar->limpiarRutasPreCargadas();
                }
            }


            #pragma omp barrier

            #pragma omp master
            {
                int contadorRequestSolicitudes = 0;
                for(auto nodo_vecino: nodos_mpi_vecinos){
                    MPI_Status s;
                    MPI_Wait(&requestRecvSolicitudes[contadorRequestSolicitudes], &s);

                    MPI_Get_count(&s, MPI_SolicitudTranspaso, &cantidadSolicitudesRecibidas[nodo_vecino]);

                    contadorRequestSolicitudes++;
                }

                int contadorRequestNotificaciones = 0;
                for(auto nodo_vecino: nodos_mpi_vecinos){
                    MPI_Status s;
                    MPI_Wait(&requestRecvNotificaciones[contadorRequestNotificaciones], &s);

                    MPI_Get_count(&s, MPI_INT, &cantidadNotificacionesRecibidas[nodo_vecino]);

                    contadorRequestNotificaciones++;
                }
            }

            #pragma omp barrier

            #pragma omp master
            {
                if ((numero_epoca + 1) % 250 == 0) {
                    MPI_Allreduce(&numero_vehiculos_en_curso_en_el_nodo, &numero_vehiculos_en_curso_global, 1, MPI_INT,
                                  MPI_SUM,
                                  MPI_COMM_WORLD);
                }
                numero_epoca++;

                MPI_Barrier(MPI_COMM_WORLD);

                for (auto s: solicitudes_transpaso_entre_nodos_mpi) {
                    solicitudes_traspaso_epoca_anterior.push_back(s);
                }
                for (auto n: notificaciones_transpaso_entre_nodos_mpi) {
                    notificaciones_traspaso_epoca_anterior.push_back(n);
                }

                solicitudes_transpaso_entre_nodos_mpi.clear();
                notificaciones_transpaso_entre_nodos_mpi.clear();

            };

            #pragma omp single
            {
                numAllSolicitudesRecividas = 0;
                numAllNotificacionesRecividas = 0;

                leer_solicitud_recibidas(allsolicitudesRecividas, numAllSolicitudesRecividas);
                leer_notificaciones_recibidas(allNotificacionesRecividas, numAllNotificacionesRecividas);

                numero_vehiculos_en_curso_en_el_nodo += numAllSolicitudesRecividas;
                numero_vehiculos_en_curso_en_el_nodo -= numAllNotificacionesRecividas;
            };


            #pragma omp single nowait
            {
                procesar_notificaciones_recividas(allNotificacionesRecividas, numAllNotificacionesRecividas);
            };

            procesar_solicitudes_recividas(allsolicitudesRecividas, numAllSolicitudesRecividas);

            #pragma omp barrier

        }while(numero_vehiculos_en_curso_global > 0);
    }
}

void initConfig(){
    omp_set_num_threads(stoi(conf["omp_set_num_threads"]));
    // ---- Configuraciones

    el::Configurations defaultConf;
    defaultConf.setToDefault();
    // Values are always std::string
    defaultConf.set(el::Level::Info,
                    el::ConfigurationType::Format, "%datetime %level %msg");
    std::string loogingFile =  PROJECT_BASE_DIR + std::string( "/logs/logs.log" );
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

void intercambiar_segmentos(map<long, vector<SegmentoTrayectoVehculoEnBarrio>> & nodos_a_enviar_mpi_por_barrio){
    map<int, int> cantidad_mensajes_a_enviar;
    int cantidad_mensajes_a_enviar_total = 0;
    for (int rank_nodo_a_enviar = 0; rank_nodo_a_enviar < size_mpi; rank_nodo_a_enviar++) {

        if(rank_nodo_a_enviar == my_rank){
            continue;
        }

        for (const auto &par: asignacion_barrios)
            if (par.second == rank_nodo_a_enviar && !nodos_a_enviar_mpi_por_barrio[par.first].empty()){
                int cantidad_segmentos = (int)nodos_a_enviar_mpi_por_barrio[par.first].size();
                cantidad_mensajes_a_enviar[rank_nodo_a_enviar] += cantidad_segmentos;

                cantidad_mensajes_a_enviar_total += cantidad_segmentos;
            }
    }

    int size_buffer = cantidad_mensajes_a_enviar_total * (int)sizeof(SegmentoTrayectoVehculoEnBarrio) + MPI_BSEND_OVERHEAD;
    auto bufferEnvioSegmentos = (char*)malloc( size_buffer);
    MPI_Buffer_attach(bufferEnvioSegmentos, size_buffer);

    auto requests_cantidad_segmentos = new MPI_Request[size_mpi - 1];
    auto buff_cantidad_segmentos = new int[size_mpi - 1];
    int cont_request = 0;

    for (int rank_nodo_a_enviar =0; rank_nodo_a_enviar < size_mpi; rank_nodo_a_enviar++) {
        if (rank_nodo_a_enviar == my_rank)
            continue;

        buff_cantidad_segmentos[cont_request] = cantidad_mensajes_a_enviar[rank_nodo_a_enviar];


        MPI_Isend(&buff_cantidad_segmentos[cont_request], 1, MPI_INT,
                  rank_nodo_a_enviar, TAG_CANTIDAD_SEGMENTOS, MPI_COMM_WORLD, &requests_cantidad_segmentos[cont_request]);

        cont_request++;
    }


    //enviar para cada nodo mpi la informacion que le corresponde.
    int max_numero_segmentos_a_recibir = 0;
    for (int rank_nodo_a_enviar = 0; rank_nodo_a_enviar < size_mpi; rank_nodo_a_enviar++) {
        if(rank_nodo_a_enviar == my_rank)
            continue;

        int cantidad_segmentos;
        MPI_Recv(&cantidad_segmentos, 1, MPI_INT, MPI_ANY_SOURCE, TAG_CANTIDAD_SEGMENTOS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        max_numero_segmentos_a_recibir = max(max_numero_segmentos_a_recibir, cantidad_segmentos);
    }

    for (int rank_nodo_a_enviar = 0; rank_nodo_a_enviar < size_mpi; rank_nodo_a_enviar++) {
        if(rank_nodo_a_enviar == my_rank)
            continue;

        auto buff_data_vehiculo_a_enviar = new SegmentoTrayectoVehculoEnBarrio[cantidad_mensajes_a_enviar[rank_nodo_a_enviar]];
        int cont = 0;

        for (auto bb: asignacion_barrios) {
            long idBarrio = bb.first;
            if (bb.second == rank_nodo_a_enviar && !nodos_a_enviar_mpi_por_barrio[idBarrio].empty()) {
                for (auto &m: nodos_a_enviar_mpi_por_barrio[idBarrio]) {
                    buff_data_vehiculo_a_enviar[cont] = m;
                    cont++;
                }
            }
        }

        MPI_Bsend(buff_data_vehiculo_a_enviar, cantidad_mensajes_a_enviar[rank_nodo_a_enviar],
                  MPI_SegmentoTrayectoVehculoEnBarrio, rank_nodo_a_enviar, TAG_SEGMENTOS, MPI_COMM_WORLD);

        delete[] buff_data_vehiculo_a_enviar;

    }



    auto buffResepcion = new SegmentoTrayectoVehculoEnBarrio[max_numero_segmentos_a_recibir]; //Todo Mejorar estimador

    for (int rank_nodo_a_enviar =0; rank_nodo_a_enviar < size_mpi; rank_nodo_a_enviar++) {
        if(rank_nodo_a_enviar == my_rank){
            continue;
        }

        MPI_Status status;
        MPI_Recv(buffResepcion, max_numero_segmentos_a_recibir, MPI_SegmentoTrayectoVehculoEnBarrio, MPI_ANY_SOURCE, TAG_SEGMENTOS, MPI_COMM_WORLD, &status);
        int cantidad_segmentos_recibidos;
        MPI_Get_count(&status, MPI_SegmentoTrayectoVehculoEnBarrio, &cantidad_segmentos_recibidos);

        if(cantidad_segmentos_recibidos > 0){
            for(int i = 0; i < cantidad_segmentos_recibidos; i++){
                pair<int, long> clave = make_pair(buffResepcion[i].id_vehiculo,
                                                  buffResepcion[i].id_barrio);
                segmentos_a_recorrer_por_barrio_por_vehiculo[clave].push(buffResepcion[i]);
                SegmentoTrayectoVehculoEnBarrio seg = buffResepcion[i];
            }
        }
    }

    delete [] buffResepcion;

    for(int i = 0; i < cont_request; i++){
        MPI_Wait(&requests_cantidad_segmentos[i], MPI_STATUS_IGNORE);
    }


    MPI_Buffer_detach(bufferEnvioSegmentos, &size_buffer);
    delete [] requests_cantidad_segmentos;
    delete [] bufferEnvioSegmentos;

}

void generar_vehiculos_y_notificar_segmentos( std::mt19937& rng, std::map<long, int> & cantidad_vehiculos_a_generar_por_barrio, std::vector<std::vector<double>> & prob_barrio_barrio, std::map<std::string, std::string> & conf, vector<pair<long, basic_string<char>>> & barrios) {

    time_point<Clock> inicioTiempoDJ = Clock::now();


    // --- Genero el inicio y final de cada nodo

    //numero_vehiculos_en_curso_en_el_nodo = numero_vehiculos_en_curso_global; //ToDo sacar cuando se haga la distribucion de vehiculos por barrios

    vector<long> nodo_inicial;
    vector<long> nodo_final;

    // ----- Sorteo los puntos de inicio y final

    numero_vehiculos_en_curso_en_el_nodo = 0;
    for (auto bario_y_cantidad: cantidad_vehiculos_a_generar_por_barrio) {
        if(asignacion_barrios[bario_y_cantidad.first] == my_rank){
            for (int i = 0; i < bario_y_cantidad.second; i++) {
                long id_barrio = bario_y_cantidad.first;
                nodo_inicial.push_back(grafoMapa->idNodoAletorio(rng, id_barrio));
                long barrioSortadoIndice = getClasePorProbailidad(rng, prob_barrio_barrio[id_barrio - 1]);
                nodo_final.push_back( grafoMapa->idNodoAletorio(rng, barrios[barrioSortadoIndice].first));




                numero_vehiculos_en_curso_en_el_nodo++;
            }
        }
    }


    // ----- Calculamos a partir de que id podemos generar los vehiculos
    int numVehiculosDeNodosAnteriores = 0;
    for (auto bario_y_cantidad: cantidad_vehiculos_a_generar_por_barrio) {
        if(asignacion_barrios[bario_y_cantidad.first] < my_rank){
            numVehiculosDeNodosAnteriores+=bario_y_cantidad.second;
        }
    }


    // ----- Calcular caminos

    map<long, vector<SegmentoTrayectoVehculoEnBarrio>> nodos_a_enviar_mpi_por_barrio;
    int numeroVehiculosFallo = 0;

    //PARA CADA VEHICULO DEFINIDO CALCULAMOS SU RUTA.
    #pragma omp parallel for schedule(dynamic, 100)
    for (int num_vehiculo_localmente_generado = 0; num_vehiculo_localmente_generado < numero_vehiculos_en_curso_en_el_nodo; num_vehiculo_localmente_generado++) {

        int id_vehiculo = num_vehiculo_localmente_generado + numVehiculosDeNodosAnteriores;

        long src = nodo_inicial[num_vehiculo_localmente_generado];
        long dst = nodo_final[num_vehiculo_localmente_generado];

        auto camino = grafoMapa->computarCaminoMasCortoUtilizandoAStar(src, dst);

        if (camino.empty()) {
            #pragma omp atomic
            numeroVehiculosFallo++;
        } else {
            long barrio_actual = grafoMapa->obtenerNodo(src)->getSeccion();
            auto nodo_inicial_barrio = camino.front();

            int size_camino = camino.size();

            for (size_t j = 0; j < size_camino; ++j) {
                long barrio = grafoMapa->obtenerNodo(camino[j])->getSeccion();
                if (barrio != barrio_actual || j == size_camino - 1) {

                    SegmentoTrayectoVehculoEnBarrio segmento{};
                    segmento.id_vehiculo = id_vehiculo;
                    segmento.id_inicio = nodo_inicial_barrio;
                    segmento.id_fin = camino[j];
                    segmento.id_barrio = barrio_actual;
                    segmento.is_segmento_final = (j == size_camino - 1) ? 1 : 0;


                    if (std::find(mis_barrios.begin(), mis_barrios.end(), barrio_actual) == mis_barrios.end()) { //El al que le corresponde el segmento no es de mi barrio no es parte de mi barrio
                        #pragma omp critical(nodos_a_enviar_mpi_mutex)
                        nodos_a_enviar_mpi_por_barrio[barrio_actual].push_back(segmento);
                    } else {
                        // Si el segmento es para el nodo que calcula, se lo guarda para si mismo
                        pair<int, long> clave;
                        clave.first = id_vehiculo;
                        clave.second = barrio_actual;

                        #pragma omp critical(segmentos_a_recorrer_pop_mutex)
                        segmentos_a_recorrer_por_barrio_por_vehiculo[clave].push(segmento);
                    }


                    barrio_actual = barrio;
                    nodo_inicial_barrio = camino[j];
                }
            }

            auto v = new Vehiculo(id_vehiculo);
            long barrio = grafoMapa->obtenerNodo(dst)->getSeccion();
            cout <<  barrio << endl;
            v->id_barrio_final = barrio;
            pair<int, long> claveBarioVehiculo = make_pair(v->getId(), grafoMapa->obtenerNodo(src)->getSeccion());


            SegmentoTrayectoVehculoEnBarrio primerSegmento = segmentos_a_recorrer_por_barrio_por_vehiculo[claveBarioVehiculo].front();

            #pragma omp critical(segmentos_a_recorrer_pop_mutex)
            segmentos_a_recorrer_por_barrio_por_vehiculo[claveBarioVehiculo].pop();

            float peso;
            auto caminoPrimerBarrio = grafoMapa->computarCaminoMasCorto(primerSegmento.id_inicio,
                                                                        primerSegmento.id_fin,
                                                                        primerSegmento.id_barrio,
                                                                        peso); //ToDo mejorar que solo busque en el barrio

            v->setRuta(caminoPrimerBarrio, primerSegmento.is_segmento_final);

            #pragma omp critical(mapa_mis_vehiculos_mutex)
            mapa_mis_vehiculos[v->getId()] = v;

            long id_camino_primer_nodo = caminoPrimerBarrio[0];
            long id_camino_segundo_nodo = caminoPrimerBarrio[1];

            string id_calle = Calle::getIdCalle(id_camino_primer_nodo, id_camino_segundo_nodo);
            Nodo *nodo_inicial_r = grafoMapa->obtenerNodo(id_camino_primer_nodo);

            Calle *calle = mapa_mis_barios[nodo_inicial_r->getSeccion()]->obtenerCalle(id_calle);

            calle->insertarSolicitudTranspaso(-1, -1, v);

        }
    }

    milliseconds milisecondsDj = duration_cast<milliseconds>(Clock::now() - inicioTiempoDJ);
    printf("----- Tiempo transcurido calculo y distribuccion caminos = %.2f seg \n",
           (float) milisecondsDj.count() / 1000.f);

    numero_vehiculos_en_curso_en_el_nodo -= numeroVehiculosFallo;

    intercambiar_segmentos(nodos_a_enviar_mpi_por_barrio);

    printf("----- Intercambio de segmentos realizado %d \n", my_rank);
}


void leerCSVbarrioCantidades(const std::string& nombreArchivo, std::map<long, int>& barrios_con_cantidades) {
    std::ifstream file(nombreArchivo);
    if (!file.is_open()) {
        std::cerr << "No se pudo abrir el archivo: " << nombreArchivo << std::endl;
        exit(1);
    }

    std::string line;

    // Omitir la primera línea de encabezado
    std::getline(file, line);

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string id_barrio_str, cantidad_str;

        std::getline(ss, id_barrio_str, ',');
        std::getline(ss, cantidad_str, ',');

        try {
            // Comprobar si las cadenas no están vacías antes de convertirlas
            if (!id_barrio_str.empty() && !cantidad_str.empty()) {
                long id_barrio = std::stol(id_barrio_str);
                int cantidad = std::stoi(cantidad_str);

                barrios_con_cantidades[id_barrio] = cantidad;
            }
        } catch (const std::invalid_argument& e) {
            std::cerr << "Error de conversión: " << e.what() << " en línea: " << line << std::endl;
        } catch (const std::out_of_range& e) {
            std::cerr << "Número fuera de rango: " << e.what() << " en línea: " << line << std::endl;
        }
    }

    file.close();
}

void calcularProbabilidad(const std::map<long, int>& barrios_con_cantidades, std::map<long, double>& probabilidad_por_barrio) {
    int total = 0;
    for (const auto& barrio : barrios_con_cantidades) {
        total += barrio.second;
    }
    for (const auto& barrio : barrios_con_cantidades) {
        probabilidad_por_barrio[barrio.first] = static_cast<double>(barrio.second) / total;
    }
}

void asignarCantidades(std::mt19937& rnd, int total_cantidad, const std::map<long, double>& probabilidad_por_barrio, std::map<long, int>& asignaciones) {

    vector<long> clavesBarrios;
    vector<double> probailidadesBarrios;

    for(auto pb: probabilidad_por_barrio){
        clavesBarrios.push_back(pb.first);
        probailidadesBarrios.push_back(pb.second);
        asignaciones[pb.first] = 0;
    }

    for(int c = 0; c < total_cantidad; c++){
        int barrioSortadoIndice = getClasePorProbailidad(rnd, probailidadesBarrios);
        asignaciones[clavesBarrios[barrioSortadoIndice]]++;
    }
}

void inicializarBuffersRecepcionSolicitudNotificacion(){
    buffRecepcionSolicitudes = new SolicitudTranspaso*[size_mpi];
    buffRecepcionNotificaciones = new int*[size_mpi];

    cantidadSolicitudesRecibidas = new int[size_mpi];
    cantidadNotificacionesRecibidas = new int[size_mpi];

    for(int nodo_index = 0; nodo_index < size_mpi; nodo_index++){
        buffRecepcionSolicitudes[nodo_index] = new SolicitudTranspaso[MAX_SOLICITUDES_NOTIFICIACION_RECIVIDAS_POR_NODO_POR_EPOCA];
        buffRecepcionNotificaciones[nodo_index] = new int[MAX_SOLICITUDES_NOTIFICIACION_RECIVIDAS_POR_NODO_POR_EPOCA];

        cantidadSolicitudesRecibidas[nodo_index] = 0;
        cantidadNotificacionesRecibidas[nodo_index] = 0;

    }

}


int main(int argc, char* argv[]) {

    std::map<long, int> barrios_con_poblacion;
    std::map<long, double> probabilidad_por_barrio;
    std::map<long, int> cantidad_vehiculos_a_generar_por_barrio;


    initConfig();
    initMpi(argc, argv);
    inicializarBuffersRecepcionSolicitudNotificacion();
    std::mt19937 rng(2024); // Semilia random


    // ---- Funciones de notificacion
    auto notificarFinalizacion = std::function<void(float, int,long)>{};
    notificarFinalizacion = [&] (float distancia, int numero_epocas, long id_barrio_final) -> void {
        numero_vehiculos_en_curso_en_el_nodo--;
        tiempoRecorridoPor10Km.push_back(  ((float)numero_epocas / distancia) * 10000);
        tiempoRecorridoPor10KmBarrio[id_barrio_final].push_back( ((float)numero_epocas / distancia) * 10000);
    };


    auto ingresarSolicitudTranspaso = std::function<void(SolicitudTranspaso&)>{};
    ingresarSolicitudTranspaso = [&] (SolicitudTranspaso& solicitud) -> void {solicitudes_transpaso_entre_nodos_mpi.push_back(solicitud);};

    auto ingresarNotificacionTranspaso = std::function<void(NotificacionTranspaso &)>{};
    ingresarNotificacionTranspaso = [&] (NotificacionTranspaso & notificacion) -> void {notificaciones_transpaso_entre_nodos_mpi.push_back(notificacion);};

    // ---- Leer MAPA
    CargarGrafo loadData = CargarGrafo(PROJECT_BASE_DIR + std::string("/datos/montevideo_por_barrios.json"));
    //CargarGrafo loadData = CargarGrafo(PROJECT_BASE_DIR + std::string("/datos/Roma_suburbio.json"));


    vector<pair<long, basic_string<char>>> barrios = loadData.obtenerBarrios();
    std::sort(barrios.begin(), barrios.end(), [](pair<long, basic_string<char>>  &a, pair<long, basic_string<char>>  &b) {
        return a.first < b.first;
    });
    int numBarrios = (int)barrios.size();
    auto buffAsignacionesBarrio = new Asignacion_barrio [numBarrios];

    std::vector<std::vector<double>> prob_barrios_a_barrios = cargar_matriz_barrios_barrios(PROJECT_BASE_DIR + std::string("/datos/probabilidad_barrios_a_barrios.json"),numBarrios);

    if(my_rank == 0){

        // ---- Leer poblaciones por barrio
        std::string dir = PROJECT_BASE_DIR + std::string("/datos/cantidad_personas_por_barrio_montevideo.csv");
        //std::string dir = PROJECT_BASE_DIR + std::string("/datos/cantidad_personas_por_barrio_roma.csv");
        leerCSVbarrioCantidades( dir ,barrios_con_poblacion);
        calcularProbabilidad(barrios_con_poblacion, probabilidad_por_barrio);
        asignarCantidades(rng, numero_vehiculos_en_curso_global, probabilidad_por_barrio, cantidad_vehiculos_a_generar_por_barrio);

        switch(stoi(conf["modo_balance_carga"])) {
            case 1:
                calculo_equitativo_por_personas(size_mpi,my_rank,cantidad_vehiculos_a_generar_por_barrio,asignacion_barrios,mis_barrios);
                break;
            case 2:
                calculo_naive_por_nodo_mpi(my_rank,size_mpi,barrios,asignacion_barrios,mis_barrios);
                break;
        }

        int contador = 0;
        for(auto as: asignacion_barrios){
            long idBarrio = as.first;

            Asignacion_barrio asignacion;
            asignacion.id_barrio = idBarrio;
            asignacion.cantidad_vehiculos_a_generar = cantidad_vehiculos_a_generar_por_barrio[idBarrio];
            asignacion.nodo_responsable = as.second;

            buffAsignacionesBarrio[contador] = asignacion;

            contador++;
        }
    }

    MPI_Bcast(buffAsignacionesBarrio, numBarrios, MPI_AsignacionBarrio, 0, MPI_COMM_WORLD);


    if(my_rank != 0){ //El nodo master ya realizo este proceso mientras determinaba la asignacion de los barrios a los nodos.
        for(int i = 0; i < numBarrios; i++){

            Asignacion_barrio asignacion = buffAsignacionesBarrio[i];

            asignacion_barrios[asignacion.id_barrio] = asignacion.nodo_responsable;
            cantidad_vehiculos_a_generar_por_barrio[asignacion.id_barrio] = asignacion.cantidad_vehiculos_a_generar;

            if(asignacion.nodo_responsable == my_rank){
                mis_barrios.push_back(asignacion.id_barrio);
            }

        }
    }


    delete [] buffAsignacionesBarrio;

    // ----- Funcion de asignacion de barrios a nodos MPI
    grafoMapa = new Grafo();
    loadData.FormarGrafo(grafoMapa,
                         mapa_mis_barios,
                         notificarFinalizacion,
                         ingresarSolicitudTranspaso,
                         ingresarNotificacionTranspaso,
                         asignacion_barrios,
                         &segmentos_a_recorrer_por_barrio_por_vehiculo,my_rank);


    calcular_nodos_mpi_vecinos(); //Calculo cuales son mis nodos MPI vecinos

    // ---- Formar un unico arreglo de Calles para todos los barrios del
    for(const auto& id_bario_y_barrio: mapa_mis_barios){
        id_bario_y_barrio.second->addCalles(todas_calles);
    }


    generar_vehiculos_y_notificar_segmentos(rng, cantidad_vehiculos_a_generar_por_barrio,prob_barrios_a_barrios,conf, barrios);

    // ---- Ejecuccion de la simulaccion

    time_point<Clock> inicioTiempo = Clock::now();

    inicioTiempoEp = Clock::now();



    ejecutar_epoca();





    // ---- Calculo del tiempo promedio de viaje

    double tiempoTotal;
    double tiempoTotalEnNodo = 0.0;

    for(float tiempo: tiempoRecorridoPor10Km){
        tiempoTotalEnNodo += tiempo;
    }
    MPI_Reduce(&tiempoTotalEnNodo, &tiempoTotal, 1, MPI_DOUBLE, MPI_SUM, 0 , MPI_COMM_WORLD);

    long cantidadTotal;
    long cantidadEnNodo = (long)tiempoRecorridoPor10Km.size();

    MPI_Reduce(&cantidadEnNodo, &cantidadTotal, 1, MPI_LONG, MPI_SUM, 0 , MPI_COMM_WORLD);

    if(my_rank == 0){
        double tiempoPromedio = tiempoTotal / (double)cantidadTotal;
        printf("----- TIEMPO PROMEDIO VIAJE DE 10KM = %2.f seg (%2.f min)\n", tiempoPromedio / 10.f, tiempoPromedio / (float)(10 * 60) );
        milliseconds miliseconds = duration_cast<milliseconds>(Clock::now() - inicioTiempo);
        printf("----- Tiempo transcurido simulacion = %.2f seg \n", (float)miliseconds.count() / 1000.f);
    }

    printf("NROBARRIO,PROMEDIO\n");
    for(auto tiempos_en_barrio : tiempoRecorridoPor10KmBarrio) {
        double tiempoTotal = 0;
        double cantidadTotal = tiempos_en_barrio.second.size();
        for(auto tiempos : tiempos_en_barrio.second ) {
            tiempoTotal += tiempos;
        }
        double tiempoPromedio = tiempoTotal / (double)cantidadTotal;
        printf("%ld,%2.f \n",tiempos_en_barrio.first, tiempoPromedio / 10.f );
    }

    // ----- Limpieza
    for (auto barrios : mapa_mis_barios)
        delete barrios.second;


    for (auto v: mapa_mis_vehiculos)
        delete v.second;

    for(int nodo_index = 0; nodo_index < size_mpi; nodo_index++){
        delete [] buffRecepcionSolicitudes[nodo_index];
        delete [] buffRecepcionNotificaciones[nodo_index];
    }
    delete [] buffRecepcionSolicitudes;
    delete [] buffRecepcionNotificaciones;
    delete [] cantidadNotificacionesRecibidas;
    delete [] cantidadSolicitudesRecibidas;

    delete grafoMapa;

    MPI_Finalize();


    return 0;

}

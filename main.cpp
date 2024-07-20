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

INITIALIZE_EASYLOGGINGPP

#include "json.hpp"
#define TIEMPO_EPOCA_MS 10 //En ms

using Clock = std::chrono::steady_clock;
using chrono::time_point;
using chrono::duration_cast;
using chrono::milliseconds;
using chrono::seconds;


using namespace std;

// ----------- Struct Auxiliares

struct cuatros_longs {
    long a;
    long b;
    long c;
    long d;
};

struct VehiculoData {
    long id_vehiculo;
    long id_inicio;
    long id_fin;
    long id_barrio;
};


// ----------- Tipos MPI

MPI_Datatype MPI_VehiculoData;




void create_mpi_types() {
    MPI_Type_contiguous(4, MPI_LONG, &MPI_VehiculoData);
    MPI_Type_commit(&MPI_VehiculoData);
}

map<long, Barrio*> mapa_mis_barios;
vector<Calle*> todas_calles;

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


    CargarGrafo loadData = CargarGrafo(PROJECT_BASE_DIR + std::string("/datos/montevideo_por_barrios.json"));
    vector<pair<long, basic_string<char>>> barrios = loadData.obtenerBarrios();

    set<long> mis_barrios;
    map<long, int> asignacion_barrios;

    //Funcion de asignacion //ToDO Cambiar
    for(int i = 0; i < barrios.size(); i++){
        asignacion_barrios[barrios[i].first] = i % size;
        if(rank == i % size){
            mis_barrios.insert(barrios[i].first);
        }
    }



    auto grafoMapa = new Grafo();
    loadData.FormarGrafo(grafoMapa, mapa_mis_barios, notificarFinalizacion, asignacion_barrios, rank);


    //Formar un unico arreglo de Calles para todos los barrios del
    for(const auto& id_bario_y_barrio: mapa_mis_barios){
        id_bario_y_barrio.second->addCalles(todas_calles);
    }



    map<int,long,long> nodos_inicio_final_vehiculo;

    vector<Vehiculo*> todos_vehiculos;

    long* nodo_inicial = new long[numeroVehiculosPendientes];
    long* nodo_final   = new long[numeroVehiculosPendientes];
    for(int i = 0 ; i < numeroVehiculosPendientes; i++) {
        nodo_inicial[i] = grafoMapa->idNodoAletorio(rng);
        nodo_final[i]   = grafoMapa->idNodoAletorio(rng);
    }


    time_point<Clock> inicioTiempoDJ = Clock::now();
    int numeroVehiculosFallo = 0;




    if (rank == 0){


        //   ----  CALCULAR CAMINOS
        cout << "Calculando caminos" << endl;



        //dado un barrio tiene un vector que contiene datos del tipo (id_vehiculo, nodo_inicial, nodo_final)
        map<long, vector<cuatros_longs>> nodos_a_enviar_mpi_por_barrio;

        //PARA CADA VEHICULO DEFINIDO CALCULAMOS SU RUTA.
        #pragma omp parallel for schedule(dynamic)
        for(int id_vehiculo = 0 ; id_vehiculo < numeroVehiculosPendientes; id_vehiculo++) {

            int thread_id = omp_get_thread_num();
            long src = nodo_inicial[id_vehiculo];
            long dst = nodo_final[id_vehiculo];

            auto camino = grafoMapa->computarCaminoMasCorto(src,dst);


            for(auto c : camino) {
                cout << grafoMapa->obtenerNodo(c)->getSeccion()<< "-" <<c << "  ";
            }
            cout << endl;

            long barrio_actual = grafoMapa->obtenerNodo(src)->getSeccion();
            auto nodo_inicial_barrio = camino.front();
            for (size_t j = 0; j < camino.size(); ++j) {
                int barrio = grafoMapa->obtenerNodo(camino[j])->getSeccion();
                if(barrio != barrio_actual) {
                    cuatros_longs cuatro{};
                    cuatro.a = (long)id_vehiculo;
                    cuatro.b = nodo_inicial_barrio;
                    cuatro.c = camino[j];
                    cuatro.d = barrio_actual;

                    nodos_a_enviar_mpi_por_barrio[barrio_actual].push_back(cuatro);

                    barrio_actual = barrio;
                    nodo_inicial_barrio = camino[j];
                }
            }
            /*
            if(camino.empty()){
                #pragma omp atomic
                numeroVehiculosFallo++;
            } else {
                auto v = new Vehiculo(id_vehiculo, 0, 45);
                v->setRuta(camino);
                todos_vehiculos.push_back(v);
                long id_camino_primer_nodo = camino[0];
                long id_camino_segundo_nodo = camino[1];

                string id_calle = Calle::getIdCalle(id_camino_primer_nodo, id_camino_segundo_nodo);
                Nodo* nodo_inicial_r = grafoMapa->obtenerNodo(id_camino_primer_nodo);
                Calle * calle = mapa_mis_barios[nodo_inicial_r->getSeccion()]->obtenerCalle(id_calle);
                calle->insertarSolicitudTranspaso(nullptr, v);
            }
            */
        }

        //enviar para cada nodo mpi la informacion que le corresponde.
        for(int rank_nodo_a_enviar = 1; rank_nodo_a_enviar < size; rank_nodo_a_enviar ++) {
            int cantidad_mensajes_a_enviar = 0;

            for (const auto &par: asignacion_barrios)
                if (par.second == rank_nodo_a_enviar && !nodos_a_enviar_mpi_por_barrio[par.first].empty())
                    cantidad_mensajes_a_enviar += nodos_a_enviar_mpi_por_barrio[par.first].size();


            //computo por cada barrio los vehiculos que le corresponden y lo agrego a la estructura de data_vehiculo_a_enviar.

            auto data_vehiculo_a_enviar = new long[cantidad_mensajes_a_enviar*4];
            int cont = 0;

            for (auto bb: asignacion_barrios) {
                long idBarrio = bb.first;
                if (bb.second == rank_nodo_a_enviar && !nodos_a_enviar_mpi_por_barrio[idBarrio].empty()) {

                    for (auto &m: nodos_a_enviar_mpi_por_barrio[idBarrio]) {
                        data_vehiculo_a_enviar[cont] =  m.a;
                        data_vehiculo_a_enviar[cont+1] =  m.b;
                        data_vehiculo_a_enviar[cont+2] =  m.c;
                        data_vehiculo_a_enviar[cont+3] =  m.d;
                        cont+=4;

                    }
                }
            }



            MPI_Send(&cantidad_mensajes_a_enviar, 1, MPI_INT, rank_nodo_a_enviar, 0, MPI_COMM_WORLD);

            MPI_Send(data_vehiculo_a_enviar, cantidad_mensajes_a_enviar * 4, MPI_LONG, rank_nodo_a_enviar, 0,
                     MPI_COMM_WORLD);

            delete[] data_vehiculo_a_enviar;
        }
    }else {
        MPI_Status status;
        int cantidad_vehiculos_barrios;

        MPI_Recv(&cantidad_vehiculos_barrios, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);

        printf("CANTIDAD: %d \n", cantidad_vehiculos_barrios);

        VehiculoData * inicio_fin_vehculos_nodo = new VehiculoData[cantidad_vehiculos_barrios];

        MPI_Recv(inicio_fin_vehculos_nodo,cantidad_vehiculos_barrios * 4,MPI_LONG,0,0,MPI_COMM_WORLD,&status);
        for(int i =0; i < cantidad_vehiculos_barrios; i ++) {
            std::cout <<"Barrio:" << inicio_fin_vehculos_nodo[i].id_barrio << " Id_vehiculo:" << inicio_fin_vehculos_nodo[i].id_vehiculo << " I: " << inicio_fin_vehculos_nodo[i].id_inicio << "F: " << inicio_fin_vehculos_nodo[i].id_fin << "\n";
        }
        cout << std::endl;
    }

    milliseconds milisecondsDj = duration_cast<milliseconds>(Clock::now() - inicioTiempoDJ);
    printf("----- Tiempo transcurido = %.2f seg \n", (float)milisecondsDj.count() / 1000.f);

    numeroVehiculosPendientes -= numeroVehiculosFallo;

    delete[] nodo_inicial;
    delete[] nodo_final;

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

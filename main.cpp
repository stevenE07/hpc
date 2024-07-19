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

INITIALIZE_EASYLOGGINGPP

#include "json.hpp"
#define TIEMPO_EPOCA_MS 10 //En ms

using Clock = std::chrono::steady_clock;
using chrono::time_point;
using chrono::duration_cast;
using chrono::milliseconds;
using chrono::seconds;


using namespace std;



map<long, Barrio*> mapa_barios;
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


void cargar_todas_calles(){
    for(auto id_y_barrio : mapa_barios){

    }
}


int main(int argc, char* argv[]) {


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

    // ---- random
    std::mt19937 rng(2024);

    LOG(INFO) << "------------COMIENZO--------------";


    // ---- Funciones de notificacion

    long numeroVehiculosPendientes = 1000;

    auto notificarFinalizacion = std::function<void()>{};
    notificarFinalizacion = [&] () -> void {numeroVehiculosPendientes--;};

    // ---- Cargar mapa




    //---- MPI
     int rank, size;

    // Inicializar MPI
    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_Comm_size(MPI_COMM_WORLD, &size);

    CargarGrafo loadData = CargarGrafo(PROJECT_BASE_DIR + std::string("/datos/montevideo_por_barrios.json"));

    vector<pair<long, basic_string<char>>> barrios = loadData.obtenerBarrios();

    set<long> mis_barrios;
    map<long, int> asignacion_barrios;

    for(int i = 0; i < barrios.size(); i++){
        asignacion_barrios[barrios[i].first] = i % size;
        if(rank == i % size){
            mis_barrios.insert(barrios[i].first);
        }
    }


    auto grafoMapa = new Grafo();
    loadData.FormarGrafo(grafoMapa, mapa_barios, notificarFinalizacion,asignacion_barrios);


    char processor_name[MPI_MAX_PROCESSOR_NAME];
    int name_len;
    MPI_Get_processor_name(processor_name, &name_len);

    printf("processor %s\n", processor_name);


    for(const auto& id_bario_y_barrio: mapa_barios){
        id_bario_y_barrio.second->addCalles(todas_calles);
    }


    vector<Vehiculo*> todos_vehiculos;

    long* nodo_inicial = new long[numeroVehiculosPendientes];
    long* nodo_final   = new long[numeroVehiculosPendientes];
    for(int i = 0 ; i < numeroVehiculosPendientes; i++) {
        nodo_inicial[i] = grafoMapa->idNodoAletorio(rng);
        nodo_final[i]   = grafoMapa->idNodoAletorio(rng);
    }



    cout << "Calculando caminos" << endl;
    time_point<Clock> inicioTiempoDJ = Clock::now();

    int numeroVehiculosFallo = 0;

    #pragma omp parallel for schedule(dynamic)
    for(int i = 0 ; i < numeroVehiculosPendientes; i++){
        int thread_id = omp_get_thread_num();
        long src = nodo_inicial[i];
        long dst = nodo_final[i];

        auto camino = grafoMapa->computarCaminoMasCorto(src,dst);
        if(camino.empty()){
            #pragma omp atomic
            numeroVehiculosFallo++;
        } else {
            auto v = new Vehiculo(i, 0, 45);
            v->setRuta(camino);
            todos_vehiculos.push_back(v);
            long id_camino_primer_nodo = camino[0];
            long id_camino_segundo_nodo = camino[1];

            string id_calle = Calle::getIdCalle(id_camino_primer_nodo, id_camino_segundo_nodo);
            Nodo* nodo_inicial_r = grafoMapa->obtenerNodo(id_camino_primer_nodo);
            Calle * calle = mapa_barios[nodo_inicial_r->getSeccion()]->obtenerCalle(id_calle);
            calle->insertarSolicitudTranspaso(nullptr, v);
        }
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

    for (auto barrios : mapa_barios){
        delete barrios.second;
    }

    for (Vehiculo* v: todos_vehiculos){
        delete v;
    }


    MPI_Finalize();


    return 0;

}

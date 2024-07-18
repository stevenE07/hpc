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



map<string, Calle*> mapa_calles;
vector<Calle *> calles;



void ejecutar_epoca(int numero_epoca, long num_vehioculo){

    if(numero_epoca % 500 == 1){
        cout << " ========  Epoca  "<< numero_epoca << " | "<< num_vehioculo << " ==========" << endl;
    }
    LOG(INFO) << " ========  Epoca  "<< numero_epoca << " ==========";
    #pragma omp parallel for shared(calles)
    for (int i = 0; i < calles.size(); ++i) {
        auto it = calles[i];
        it->ejecutarEpoca(TIEMPO_EPOCA_MS); // Ejecutar la época para la calle
        if (numero_epoca % 10 == 0) {
            //#pragma omp critical
            //it->mostrarEstado(); // Mostrar el estado cada 10 épocas
        }
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

    auto getCalle = std::function<Calle*(string)>{};
    getCalle = [=] (string idCalle) -> Calle* {return mapa_calles[idCalle];};

    long numeroVehiculosPendientes = 1000;

    auto notificarFinalizacion = std::function<void()>{};
    notificarFinalizacion = [&] () -> void {numeroVehiculosPendientes--;};

    // ---- Cargar mapa

    CargarGrafo c = CargarGrafo(PROJECT_BASE_DIR + std::string("/datos/montevideo_por_barrios.json"));
    auto grafoMapa = new Grafo();
    c.leerDatos(grafoMapa, mapa_calles, getCalle, notificarFinalizacion);


    //---- MPI

     int rank, size;

    // Inicializar MPI
    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_Comm_size(MPI_COMM_WORLD, &size);


    char processor_name[MPI_MAX_PROCESSOR_NAME];
    int name_len;
    MPI_Get_processor_name(processor_name, &name_len);

    printf("Hello world from process %d of %d on processor %s\n", rank, size, processor_name);
    // Finalizar MPI
    MPI_Finalize();

    return 0;

    /*
    for(const auto& calle_id_calle: mapa_calles){
        calles.push_back(calle_id_calle.second);
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
            Calle * calle = mapa_calles[Calle::getIdCalle(id_camino_primer_nodo, id_camino_segundo_nodo)];
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



    for (auto c : mapa_calles){
        delete c.second;
    }

    for (Vehiculo* v: todos_vehiculos){
        delete v;
    }
    */

    return 0;

}

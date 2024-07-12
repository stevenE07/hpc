#include "easylogging++.h"
#include <iostream>
#include "map"
#include "vector"
#include "include/Calle.h"
#include <filesystem>
#include "include/CargarGrafo.h"
#include "include/Grafo.h"

#include <ctime>
#include "chrono"

INITIALIZE_EASYLOGGINGPP

#include "json.hpp"
#define TIEMPO_EPOCA_MS 100 //En ms

using Clock = std::chrono::steady_clock;
using chrono::time_point;
using chrono::duration_cast;
using chrono::milliseconds;
using chrono::seconds;


using namespace std;



map<string, Calle*> todas_calles;



void ejecutar_epoca(int numero_epoca, long num_vehioculo){

    if(numero_epoca % 1000 == 0){
        cout << " ========  Epoca  "<< numero_epoca << " | "<< num_vehioculo << " ==========" << endl;
    }
    LOG(INFO) << " ========  Epoca  "<< numero_epoca << " ==========";
    for(auto calle: todas_calles){
        calle.second->ejecutarEpoca(TIEMPO_EPOCA_MS);
        calle.second->mostrarEstado();
    }
}


int main() {

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

    LOG(INFO) << "------------COMIENZO--------------";

    // ---- Funciones de notificacion

    auto getCalle = std::function<Calle*(string)>{};
    getCalle = [=] (string idCalle) -> Calle* {return todas_calles[idCalle];};

    long numeroVehiculosPendientes = 100;

    auto notificarFinalizacion = std::function<void()>{};
    notificarFinalizacion = [&] () -> void {numeroVehiculosPendientes--;};

    // ---- Cargar mapa

    CargarGrafo c = CargarGrafo(PROJECT_BASE_DIR + std::string("/datos/montevideo.json"));
    auto grafoMapa = new Grafo();
    c.leerDatos(grafoMapa, todas_calles, getCalle, notificarFinalizacion);

    vector<Vehiculo*> vec;

    for(int i = 0 ; i < numeroVehiculosPendientes; i++){

        long src = grafoMapa->idNodoAletorio();
        long dst = grafoMapa->idNodoAletorio();

        auto camino = grafoMapa->computarCaminoMasCorto(src,dst);
        while(camino.empty()){
            src = grafoMapa->idNodoAletorio();
            dst = grafoMapa->idNodoAletorio();
            cout << i <<" - Fallo" << endl;
            camino = grafoMapa->computarCaminoMasCorto(grafoMapa->idNodoAletorio(),grafoMapa->idNodoAletorio());
        }
        auto v = new Vehiculo(i, 0, 45);
        v->setRuta(camino);
        vec.push_back(v);
        long id_camino_primer_nodo = camino[0];
        long id_camino_segundo_nodo = camino[1];
        Calle * calle = todas_calles[Calle::getIdCalle(id_camino_primer_nodo, id_camino_segundo_nodo)];
        calle->insertarSolicitudTranspaso(nullptr, v);

    }

    time_point<Clock> inicioTiempo = Clock::now();

    int contador_numero_epoca = 1;
    while(numeroVehiculosPendientes > 0){
        ejecutar_epoca(contador_numero_epoca, numeroVehiculosPendientes);
        contador_numero_epoca++;
    }

    milliseconds miliseconds = duration_cast<milliseconds>(Clock::now() - inicioTiempo);
    printf("----- Tiempo transcurido = %.2f seg \n", (float)miliseconds.count() / 1000.f);

    for (auto c : todas_calles){
        delete c.second;
    }

    for (Vehiculo* v: vec){
        delete v;
    }
    return 0;

}

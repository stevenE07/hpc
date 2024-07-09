#include "easylogging++.h"
#include <iostream>
#include <functional>
#include "map"
#include "vector"
#include "include/Calle.h"
#include "iostream"
#include <filesystem>
#include "include/CargarGrafo.h"
#include "include/Grafo.h"

INITIALIZE_EASYLOGGINGPP

#include "json.hpp"
#define TIEMPO_EPOCA_MS 100 //En ms



using namespace std;



map<string, Calle*> todas_calles;



void ejecutar_epoca(int numero_epoca){

    cout << " ========  Epoca  "<< numero_epoca << " ==========" << endl;
    LOG(INFO) << " ========  Epoca  "<< numero_epoca << " ==========";
    for(auto calle: todas_calles){
        calle.second->ejecutarEpoca(TIEMPO_EPOCA_MS);
        calle.second->mostrarEstado();
    }
}


void pruebaDijkstra(){
    auto graf = new Grafo;

    int id_0 = graf->agregarNodo();
    int id_1 = graf->agregarNodo();
    int id_2 = graf->agregarNodo();
    int id_3 = graf->agregarNodo();
    int id_4 = graf->agregarNodo();

    graf->agregarArista(id_0, id_1, 1.f);
    graf->agregarArista(id_0, id_2, 3.f);
    graf->agregarArista(id_1, id_2, 1.f);
    graf->agregarArista(id_1, id_3, 1.f);
    graf->agregarArista(id_2, id_3, 4.f);
    graf->agregarArista(id_2, id_4, 2.f);
    graf->agregarArista(id_3, id_4, 1.f);

    vector<long> camino =  graf->computarCaminoMasCorto(id_0, id_4);

    for(int id: camino){
        cout << id << " ";
    }

    delete graf;
}

int main() {

    //pruebaDijkstra();
    //return 0;

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

    auto getCalle = std::function<Calle*(string)>{};
    getCalle = [=] (string idCalle) -> Calle* {return todas_calles[idCalle];};

    CargarGrafo c = CargarGrafo(PROJECT_BASE_DIR + std::string("/datos/calles_montevideo.json"));
    auto grafoMapa = new Grafo();

    long numeroVehiculosPendientes = 1;

    auto notificarFinalizacion = std::function<void()>{};
    notificarFinalizacion = [&] () -> void {numeroVehiculosPendientes--;};

    c.leerDatos(grafoMapa, todas_calles, getCalle, notificarFinalizacion);

    vector<long> camino = grafoMapa->computarCaminoMasCorto(20, 219);
    for(long c: camino){
        cout << c << endl;
    }

    vector<Vehiculo*> vec;

    auto v = new Vehiculo(0, 0, 45);
    v->setRuta(camino);
    vec.push_back(v);

    long id_camino_primer_nodo = camino[0];
    long id_camino_segundo_nodo = camino[1];
    Calle* primeraCalle = todas_calles[Calle::getIdCalle(id_camino_primer_nodo, id_camino_segundo_nodo)];
    primeraCalle->insertarSolicitudTranspaso(nullptr, v);

    int cont = 1;
    while(numeroVehiculosPendientes > 0){
        ejecutar_epoca(cont);
        cont++;
    }

    for (auto c : todas_calles){
        delete c.second;
    }

    for (Vehiculo* v: vec){
        delete v;
    }
    return 0;

}

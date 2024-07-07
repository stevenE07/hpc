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

    vector<int> camino =  graf->computarCaminoMasCorto(id_0, id_4);

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

    CargarGrafo c = CargarGrafo(PROJECT_BASE_DIR + std::string("/datos/calles_montevideo.json"));
    /*
    vector<Vehiculo*> vec;

    auto getCalle = std::function<Calle*(string)>{};
    getCalle = [=] (string idCalle) -> Calle* {return todas_calles[idCalle];};

    auto calle_01 = new Calle(0,1,100,2,45, getCalle);
    for(int j = 0; j<8;j++){
        vector<unsigned int > ruta1 ;
        ruta1.push_back(0);
        ruta1.push_back(1);
        ruta1.push_back(2);
        auto v = new Vehiculo(j, 0, 45);
        v->setRuta(ruta1);
        vec.push_back(v);
        calle_01->insertarSolicitudTranspaso(nullptr, v);
    }

    auto calle_31 = new Calle(3,1,100,2,45, getCalle);
    for(int j = 0; j<8;j++){
        vector<unsigned int > ruta2 ;
        ruta2.push_back(3);
        ruta2.push_back(1);
        ruta2.push_back(2);
        auto v = new Vehiculo(j + 101, 0, 45);
        v->setRuta(ruta2);
        vec.push_back(v);
        calle_31->insertarSolicitudTranspaso(nullptr, v);
    }

    todas_calles[Calle::getIdCalle(calle_01)] = calle_01;
    todas_calles[Calle::getIdCalle(calle_31)] = calle_31;

    auto calle2 = new Calle(1,2,100,2,45, getCalle);
    todas_calles[Calle::getIdCalle(calle2)] = calle2;

    for(int i = 0; i < 500; i++){
        ejecutar_epoca(i);
    }

    for (auto c : todas_calles){
        delete c.second;
    }

    for (Vehiculo* v: vec){
        delete v;
    }
*/
    return 0;

}

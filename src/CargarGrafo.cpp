#include "../include/CargarGrafo.h"
#include "map"

#include <iostream>

CargarGrafo::CargarGrafo(std::string file) {
    std::ifstream f(file);
    data = json::parse(f);



}


void CargarGrafo::leerDatos(Grafo* grafo, std::map<std::string, Calle*>& calles, function<Calle*(string)> & obtenerCallePorIdFn, std::function<void()>& doneFn){

    map<long, pair<float, float>> corr_nodos;

    for (size_t i = 0; i < data["nodes"].size(); ++i) {
        long id_nodo_archivo = data["nodes"][i]["id"];
        grafo->agregarNodo(id_nodo_archivo);

        pair<double, double> corr;
        corr.first = data["nodes"][i]["x"];
        corr.second = data["nodes"][i]["y"];

        corr_nodos[id_nodo_archivo] = corr;
    }

    for (size_t i = 0; i < data["links"].size(); ++i) {
        long id_src = data["links"][i]["source"];
        long id_dst = data["links"][i]["target"];


        if(!grafo->existeNodo(id_src) || !grafo->existeNodo(id_dst)){
            continue;
        }

        bool invertido = false;
        bool doble = false;

        if(data["links"][i].contains("oneway")){
            doble = !data["links"][i]["oneway"];
        }

        if(!doble){
            if(data["links"][i].contains("geometry")){

                if(data["links"][i]["geometry"]["type"] != "LineString"){
                    cout<< "No soportado" << endl;
                    exit(3);
                }

                float x_i = data["links"][i]["geometry"]["coordinates"][0][0];
                float y_i = data["links"][i]["geometry"]["coordinates"][0][1];

                auto corr_nodo_src = corr_nodos[id_src];

                if ( x_i != corr_nodo_src.first || y_i != corr_nodo_src.second){

                    auto corr_nodo_dst = corr_nodos[id_dst];
                    if ( x_i != corr_nodo_dst.first || y_i != corr_nodo_dst.second){
                        cout<< "Error en las corr" << endl;
                        exit(10);
                    }
                    invertido = true;
                }
            } else if(data["links"][i].contains("reversed")){
                if (data["links"][i]["reversed"].is_array()) {
                    doble = true;
                } else {
                    invertido = data["links"][i]["reversed"];
                }

            }
        }


        if(invertido){
            swap(id_src, id_dst);
        }

        float largo = data["links"][i]["length"];

        string velocidad_max;
        if( data["links"][i].contains("maxspeed")){

            if(data["links"][i]["maxspeed"].is_string()){
                velocidad_max = data["links"][i]["maxspeed"];
            } else if (data["links"][i]["maxspeed"].is_array()) {
                velocidad_max = data["links"][i]["maxspeed"][0];
            } else {
                velocidad_max = "45";
            }
        } else {
            velocidad_max = "45";
        }

        int numeroCarriles = 1; //ToDo pendiente de leer

        if(doble){
            grafo->agregarArista(id_src, id_dst, largo);

            auto calle1 = new Calle(id_src,
                                    id_dst,
                                   largo,
                                   numeroCarriles,
                                   stof(velocidad_max),
                                   obtenerCallePorIdFn,
                                    doneFn);
            calles[Calle::getIdCalle(calle1)] = calle1;

            grafo->agregarArista(id_dst, id_src, largo);

            auto calle2 = new Calle(id_dst,
                                    id_src,
                                   largo,
                                   numeroCarriles,
                                   stof(velocidad_max),
                                   obtenerCallePorIdFn,
                                    doneFn);
            calles[Calle::getIdCalle(calle2)] = calle2;
        } else {
            grafo->agregarArista(id_src, id_dst, largo);

            auto calle = new Calle(id_src,
                                   id_dst,
                                   largo,
                                   numeroCarriles,
                                   stof(velocidad_max),
                                   obtenerCallePorIdFn,
                                   doneFn);
            calles[Calle::getIdCalle(calle)] = calle;
        }
    }
}

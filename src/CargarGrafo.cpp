#include "../include/CargarGrafo.h"
#include "map"

#include <iostream>

CargarGrafo::CargarGrafo(std::string file) {
    std::ifstream f(file);
    data = json::parse(f);



}


void CargarGrafo::leerDatos(Grafo* grafo, std::map<std::string, Calle*>& calles, function<Calle*(string)> & obtenerCallePorIdFn, std::function<void()>& doneFn){

    map<long, long> mapeo_id_datos_id_nodos_grafos;

    for (size_t i = 0; i < data["nodes"].size(); ++i) {
        long id_nodo = grafo->agregarNodo();
        long id_nodo_archivo = data["nodes"][i]["id"];

        mapeo_id_datos_id_nodos_grafos[id_nodo_archivo] = id_nodo;
        if (id_nodo == 20){
            cout << id_nodo_archivo << endl;
        }
    }

    for (size_t i = 0; i < data["links"].size(); ++i) {
        long id_src = data["links"][i]["source"];
        long id_dst = data["links"][i]["target"];

        bool invertido = false;
        bool doble = false;

        if(data["links"][i].contains("oneway")){
            doble = !data["links"][i]["oneway"];
        }

        if(!doble){
            if(data["links"][i].contains("reversed")){
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


        long id_nodo_src = mapeo_id_datos_id_nodos_grafos[id_src];
        long id_nodo_dst = mapeo_id_datos_id_nodos_grafos[id_dst];

        if(doble){
            grafo->agregarArista(id_nodo_src, id_nodo_dst, largo);

            auto calle1 = new Calle(id_nodo_src,
                                   id_nodo_dst,
                                   largo,
                                   numeroCarriles,
                                   stof(velocidad_max),
                                   obtenerCallePorIdFn,
                                    doneFn);
            calles[Calle::getIdCalle(calle1)] = calle1;

            grafo->agregarArista(id_nodo_dst, id_nodo_src, largo);

            auto calle2 = new Calle(id_nodo_dst,
                                    id_nodo_src,
                                   largo,
                                   numeroCarriles,
                                   stof(velocidad_max),
                                   obtenerCallePorIdFn,
                                    doneFn);
            calles[Calle::getIdCalle(calle2)] = calle2;
        } else {
            grafo->agregarArista(id_nodo_src, id_nodo_dst, largo);

            auto calle = new Calle(id_nodo_src,
                                   id_nodo_dst,
                                   largo,
                                   numeroCarriles,
                                   stof(velocidad_max),
                                   obtenerCallePorIdFn,
                                   doneFn);
            calles[Calle::getIdCalle(calle)] = calle;
        }
    }
}

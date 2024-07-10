#include "../include/CargarGrafo.h"
#include "map"

#include <iostream>

CargarGrafo::CargarGrafo(std::string file) {
    std::ifstream f(file);
    data = json::parse(f);



}


void CargarGrafo::leerDatos(Grafo* grafo, std::map<std::string, Calle*>& calles, function<Calle*(string)> & obtenerCallePorIdFn, std::function<void()>& doneFn){

    for (size_t i = 0; i < data["nodes"].size(); ++i) {
        long id_nodo_archivo = data["nodes"][i]["id"];
        if(id_nodo_archivo < 0){
            cout << id_nodo_archivo << endl;
        }
        grafo->agregarNodo(id_nodo_archivo);
    }

    for (size_t i = 0; i < data["links"].size(); ++i) {
        long id_src = data["links"][i]["source"];
        long id_dst = data["links"][i]["target"];




        if(!grafo->existeNodo(id_src) || !grafo->existeNodo(id_dst)){
            continue;
        }

        bool invertido = false;
        bool doble = true;

        /*if(data["links"][i].contains("oneway")){
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
        }*/


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

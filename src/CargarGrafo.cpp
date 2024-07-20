#include "../include/CargarGrafo.h"
#include "map"

#include <iostream>

CargarGrafo::CargarGrafo(string file) {
    std::ifstream f(file);
    data = json::parse(f);
}


vector<pair<long, string>> CargarGrafo::obtenerBarrios(){
    vector<pair<long, string>> barrios;

    for (size_t i = 0; i < data["suburb"].size(); ++i){
        pair<long, string> barrio;

        barrio.first = data["suburb"][i]["id"];
        barrio.second = data["suburb"][i]["name"];

        barrios.push_back(barrio);
    }

    return barrios;
}



void CargarGrafo::FormarGrafo(Grafo* grafo,
                              map<long, Barrio*>& barrios,
                              std::function<void()>& doneFn,
                              function<void(SolicitudTranspaso&)>& enviarSolicitudFn,
                              map<long, int> & asignacion_barrios,
                              int my_rank){

    map<long, pair<float, float>> mapa_corr_por_nodo;
    map<long, long> mapa_barios_por_nodo;

    for (size_t i = 0; i < data["nodes"].size(); ++i) {
        long id_nodo_archivo = data["nodes"][i]["id"];
        long barrio_id = data["nodes"][i]["suburb_id"];

        grafo->agregarNodo(id_nodo_archivo, barrio_id);
        mapa_barios_por_nodo[id_nodo_archivo] = barrio_id;

        // Solo cargo los barrios de el nodo actual
        if(barrios.find(barrio_id) == barrios.end() && asignacion_barrios[barrio_id] == my_rank){
            auto barrio = new Barrio(barrio_id);
            barrios[barrio_id] = barrio;
        }

        pair<double, double> corr;
        corr.first = data["nodes"][i]["x"];
        corr.second = data["nodes"][i]["y"];

        mapa_corr_por_nodo[id_nodo_archivo] = corr;
    }


    for (size_t i = 0; i < data["links"].size(); ++i) {
        long id_src = data["links"][i]["source"];
        long id_dst = data["links"][i]["target"];

        long id_barrio_src = mapa_barios_por_nodo[id_src];
        long id_barrio_dst = mapa_barios_por_nodo[id_dst];

        if(!grafo->existeNodo(id_src) || !grafo->existeNodo(id_dst)){
            continue;
        }

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

        if (largo < LARGO_VEHICULO) {
            largo = LARGO_VEHICULO + 1.f;
        }

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

        int numeroCarriles =  1; // data["links"][i]["lanes"];

        if(doble){

            //--- Cargo la primera arista


            if(!grafo->existeArista(id_src, id_dst)){ //No agregamos aristas repetidas
                grafo->agregarArista(id_src, id_dst, largo);

                if(asignacion_barrios[id_barrio_src] == my_rank){
                    Barrio* barrio1 = barrios[id_barrio_src];

                    if(id_barrio_src != id_barrio_dst){
                        barrio1->agregarBarrioVecino(id_barrio_dst);
                    }

                    auto calle1 = new Calle(id_src,
                                            id_dst,
                                            largo,
                                            numeroCarriles,
                                            stof(velocidad_max),
                                            barrios,
                                            doneFn,
                                            enviarSolicitudFn,
                                            grafo,
                                            asignacion_barrios);

                    barrio1->agregarCalle(calle1);
                }
            }

            //--- Cargo la segunda arista

            if(!grafo->existeArista(id_dst, id_src)) { //No agregamos aristas repetidas
                grafo->agregarArista(id_dst, id_src, largo);
                if(asignacion_barrios[id_barrio_dst] == my_rank) {
                    Barrio* barrio2 = barrios[id_barrio_dst];

                    if(id_barrio_src != id_barrio_dst){
                        barrio2->agregarBarrioVecino(id_barrio_src);
                    }

                    auto calle2 = new Calle(id_dst,
                                            id_src,
                                            largo,
                                            numeroCarriles,
                                            stof(velocidad_max),
                                            barrios,
                                            doneFn,
                                            enviarSolicitudFn,
                                            grafo,
                                            asignacion_barrios);
                    barrio2->agregarCalle(calle2);
                }
            }



        } else {

            if(!grafo->existeArista(id_src, id_dst)) { //No agregamos aristas repetidas
                grafo->agregarArista(id_src, id_dst, largo);
                if(asignacion_barrios[id_barrio_src] == my_rank){
                    Barrio* barrio = barrios[id_barrio_src];

                    if(id_barrio_src != id_barrio_dst){
                        barrio->agregarBarrioVecino(id_barrio_dst);
                    }

                    auto calle = new Calle(id_src,
                                           id_dst,
                                           largo,
                                           numeroCarriles,
                                           stof(velocidad_max),
                                           barrios,
                                           doneFn,
                                           enviarSolicitudFn,
                                           grafo,
                                           asignacion_barrios);
                    barrio->agregarCalle(calle);
                }
            }

        }
    }
}

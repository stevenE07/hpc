#include "map"
#include "vector"
#include "Nodo.h"
#include "iostream"
#include "random"
#include "Utils.h"

#ifndef GRAFO_H
#define GRAFO_H

using namespace std;

class Grafo {
private:
    int sigIdNodo = 0;
    map<long, Nodo*> nodos_id_ext;
    map<int, Nodo*> nodos_id_int;

    map<int, long> id_int_to_ext;
    map<long, int> id_ext_to_int;

    map<long, vector<long>> nodos_id_por_seccion;

public:
    Grafo();
    void agregarNodo(long id_ext, long seccion, float x, float y);
    void agregarArista(long id_ext_entrada, long id_ext_salida, float peso);

    vector<long> computarCaminoMasCorto(long id_ext_nodo_inicio, long id_ext_nodo_final, long seccion, float & pesoEncontrado);

    vector<long> computarCaminoMasCortoUtilizandoAStar(long id_ext_nodo_inicio, long id_ext_nodo_final);

    bool existeNodo(long id_ext){
        return this->nodos_id_ext.find(id_ext) != this->nodos_id_ext.end();
    }

    Nodo* obtenerNodo(long id_ext_nodo);

    vector<Nodo*> obtenerNodos();

    long idNodoAletorio(std::mt19937& rnd, long seccion){
        std::uniform_int_distribution<int> dist(0, (int)nodos_id_por_seccion[seccion].size()-1);
        return nodos_id_por_seccion[seccion][dist(rnd)];
    }

    long idNodoAletorio(std::mt19937& rnd){
        std::uniform_int_distribution<int> dist(0, sigIdNodo - 1);
        int d = dist(rnd);
        return id_int_to_ext[d];
    }

    bool existeArista(long id_ext_src, long id_ext_dst){
        if(nodos_id_ext.find(id_ext_src) != nodos_id_ext.end()){
            Nodo* nodoSrc = nodos_id_ext[id_ext_src];
            return nodoSrc->isConectadoCon(id_ext_dst);
        } else {
            //No exite el nodo src
            return false;
        }

    }

    ~Grafo();
};





#endif //GRAFO_H

#include "map"
#include "vector"
#include "Nodo.h"
#include "iostream"
#include "random"

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

public:
    Grafo();
    void agregarNodo(long id_ext);
    void agregarArista(long id_ext_entrada, long id_ext_salida, float peso);

    vector<long> computarCaminoMasCorto(long id_ext_nodo_inicio, long id_ext_nodo_final);

    //vector<long> bfs(long id_ext_nodo_inicial, float peso_minimo, float peso_maximo, float random);

    bool existeNodo(long id_ext){
        return this->nodos_id_ext.find(id_ext) != this->nodos_id_ext.end();
    }

    long idNodoAletorio(std::mt19937& rnd){

        std::uniform_int_distribution<int> dist(1, 100);
        return id_int_to_ext[dist(rnd)]; //ToDO sacar
    }

    ~Grafo();
};





#endif //GRAFO_H

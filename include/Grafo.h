#include "map"
#include "vector"
#include "Nodo.h"

#ifndef GRAFO_H
#define GRAFO_H

using namespace std;

class Grafo {
private:
    long sigNumeroId;
    map<long, Nodo*> nodos;

public:
    Grafo();
    long agregarNodo();
    void agregarArista(long id_entrada, long id_salida, float peso);

    vector<long> computarCaminoMasCorto(long id_nodo_inicio, long id_nodo_final);

    Nodo* getNodo(long clave);

    ~Grafo();
};





#endif //GRAFO_H

#include "map"
#include "vector"
#include "Nodo.h"

#ifndef GRAFO_H
#define GRAFO_H

using namespace std;

class Grafo {
private:
    int sigNumeroId;
    map<int, Nodo*> nodos;

public:
    Grafo();
    int agregarNodo();
    void agregarArista(int id_entrada, int id_salida, float peso);

    vector<int> computarCaminoMasCorto(int id_nodo_inicio, int id_nodo_final);

    ~Grafo();
};





#endif //GRAFO_H

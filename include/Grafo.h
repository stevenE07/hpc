#include "map"
#include "vector"
#include "Nodo.h"
#include "iostream"

#ifndef GRAFO_H
#define GRAFO_H

using namespace std;

class Grafo {
private:
    map<long, Nodo*> nodos;

public:
    Grafo();
    void agregarNodo(long id);
    void agregarArista(long id_entrada, long id_salida, float peso);

    vector<long> computarCaminoMasCorto(long id_nodo_inicio, long id_nodo_final);

    bool existeNodo(long clave){
        return this->nodos.find(clave) != this->nodos.end();
    }

    Nodo* getNodo(long clave);

    void printGrafo(){
        for(auto n: nodos){
            std::cout << n.second->getId() << " : ";
            for(auto v: n.second->getNodosVecinos()){
                std::cout << v.first->getId() << " ";
            }
            std::cout << endl;
        }
    }

    ~Grafo();
};





#endif //GRAFO_H

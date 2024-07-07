#include "../include/Grafo.h"
#include "algorithm"
#include "set"

Grafo::Grafo() {
    sigNumeroId = 0;
}

int Grafo::agregarNodo(){
    int id = sigNumeroId;
    sigNumeroId++;
    auto nuevo_nodo = new Nodo(id);
    nodos[id] = nuevo_nodo;
    return id;
}

void Grafo::agregarArista(int id_entrada, int id_salida, float peso){
    Nodo* nodo_entrada = nodos[id_entrada];
    Nodo* nodo_salida  = nodos[id_salida];
    nodo_entrada->conectarNodo(nodo_salida, peso);
}

vector<int> Grafo::computarCaminoMasCorto(int id_nodo_inicio, int id_nodo_final){

    if(id_nodo_inicio == id_nodo_final){
        vector<int> emptyVector;
        return emptyVector;
    }

    set<int> nodos_pendientes;
    auto pesos = new float[nodos.size()];
    auto nodo_anterior = new int[nodos.size()];

    for(auto a: nodos){
        Nodo* nodo = a.second;
        int id_nodo = nodo->getId();

        if(id_nodo == id_nodo_inicio){
            pesos[id_nodo] = 0;
        } else {
            nodos_pendientes.insert(id_nodo);
            pesos[id_nodo] = -1;
        }

        nodo_anterior[id_nodo] = -1;

    }

    int id_ultimo_nodo_agregado = id_nodo_inicio;

    while (id_ultimo_nodo_agregado != id_nodo_final){
        Nodo* ultimo_nodo_revisado = nodos[id_ultimo_nodo_agregado];
        for (auto nodo_peso_vecino: ultimo_nodo_revisado->getNodosVecinos()){
            float peso_cantidado = pesos[id_ultimo_nodo_agregado] + nodo_peso_vecino.second;
            float peso_actual_vecino = pesos[nodo_peso_vecino.first->getId()];
            if (peso_actual_vecino == -1 ||  peso_cantidado < peso_actual_vecino){
                pesos[nodo_peso_vecino.first->getId()] = peso_cantidado;
                nodo_anterior[nodo_peso_vecino.first->getId()] = id_ultimo_nodo_agregado;
            }
        }

        int min_nodo_peso = -1;
        for (int id_nodo_pendiente : nodos_pendientes){
            if(pesos[id_nodo_pendiente] != -1){
                if(min_nodo_peso == -1 || pesos[id_nodo_pendiente] < pesos[min_nodo_peso]){
                    min_nodo_peso = id_nodo_pendiente;
                }
            }
        }

        nodos_pendientes.erase(min_nodo_peso);
        id_ultimo_nodo_agregado = min_nodo_peso;
    }

    vector<int> camino;
    camino.push_back(id_nodo_final);
    int ultimo_nodo_camino = id_nodo_final;
    while(ultimo_nodo_camino != id_nodo_inicio){
        ultimo_nodo_camino = nodo_anterior[ultimo_nodo_camino];
        camino.push_back(ultimo_nodo_camino);
    }

    reverse(camino.begin(), camino.end());

    delete[] pesos;
    delete[] nodo_anterior;

    return camino;
}


Grafo::~Grafo() {
    for (auto n: nodos){
        delete(n.second);
    }
}
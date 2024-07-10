#include "../include/Grafo.h"
#include "algorithm"
#include "set"
#include "iostream"

Grafo::Grafo() {}

void Grafo::agregarNodo(long id){
    auto nuevo_nodo = new Nodo(id);
    nodos[id] = nuevo_nodo;
}

void Grafo::agregarArista(long id_entrada, long id_salida, float peso){
    Nodo* nodo_entrada = nodos[id_entrada];
    Nodo* nodo_salida  = nodos[id_salida];
    nodo_entrada->conectarNodo(nodo_salida, peso);
}

vector<long> Grafo::computarCaminoMasCorto(long id_nodo_inicio, long id_nodo_final){

    if(id_nodo_inicio == id_nodo_final){
        vector<long> emptyVector;
        return emptyVector;
    }

    set<long> nodos_pendientes;
    map<long, double> pesos;
    map<long, long> nodo_anterior;

    for(auto a: nodos){
        Nodo* nodo = a.second;
        long id_nodo = nodo->getId();

        if(id_nodo == id_nodo_inicio){
            pesos[id_nodo] = 0;
        } else {
            nodos_pendientes.insert(id_nodo);
            pesos[id_nodo] = -1;
        }

        nodo_anterior[id_nodo] = -1;

    }

    long id_ultimo_nodo_agregado = id_nodo_inicio;

    while (!nodos_pendientes.empty()){
    //while (id_ultimo_nodo_agregado != id_nodo_final){
        Nodo* ultimo_nodo_revisado = nodos[id_ultimo_nodo_agregado];
        if(ultimo_nodo_revisado == nullptr){
            std::cout << id_ultimo_nodo_agregado << endl;
        }
        if(!ultimo_nodo_revisado->getNodosVecinos().empty()){
            for (auto nodo_peso_vecino: ultimo_nodo_revisado->getNodosVecinos()){
                float peso_cantidado = pesos[id_ultimo_nodo_agregado] + nodo_peso_vecino.second;
                float peso_actual_vecino = pesos[nodo_peso_vecino.first->getId()];
                if (peso_actual_vecino == -1 ||  peso_cantidado < peso_actual_vecino){
                    pesos[nodo_peso_vecino.first->getId()] = peso_cantidado;
                    nodo_anterior[nodo_peso_vecino.first->getId()] = id_ultimo_nodo_agregado;
                }
            }
        }


        long min_nodo_peso = -1;
        for (long id_nodo_pendiente : nodos_pendientes){
            if(pesos[id_nodo_pendiente] != -1){
                if(min_nodo_peso == -1 || pesos[id_nodo_pendiente] < pesos[min_nodo_peso]){
                    min_nodo_peso = id_nodo_pendiente;
                }
            }
        }

        if(min_nodo_peso == -1){
            vector<long> emptyVector;
            return emptyVector;
        }

        nodos_pendientes.erase(min_nodo_peso);
        id_ultimo_nodo_agregado = min_nodo_peso;
    }

    vector<long> camino;
    camino.push_back(id_nodo_final);
    long ultimo_nodo_camino = id_nodo_final;
    while(ultimo_nodo_camino != id_nodo_inicio){
        ultimo_nodo_camino = nodo_anterior[ultimo_nodo_camino];
        camino.push_back(ultimo_nodo_camino);
    }

    reverse(camino.begin(), camino.end());

    return camino;
}


Grafo::~Grafo() {
    for (auto n: nodos){
        delete(n.second);
    }
}

Nodo* Grafo::getNodo(long clave){
    return nodos[clave];
}

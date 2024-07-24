#include "../include/Grafo.h"
#include "algorithm"
#include "set"
#include "queue"
#include "iostream"

Grafo::Grafo() {}

void Grafo::agregarNodo(long id_ext, long seccion, float x, float y){
    int id_int = sigIdNodo;
    sigIdNodo++;

    auto nuevo_nodo = new Nodo(id_int, id_ext,seccion, x, y);
    nodos_id_int[id_int] = nuevo_nodo;
    nodos_id_ext[id_ext] = nuevo_nodo;

    id_int_to_ext[id_int] = id_ext;
    id_ext_to_int[id_ext] = id_int;

    nodos_id_por_seccion[seccion].push_back(id_ext);
}

void Grafo::agregarArista(long id_ext_entrada, long id_ext_salida, float peso){
    Nodo* nodo_entrada = nodos_id_ext[id_ext_entrada];
    Nodo* nodo_salida  = nodos_id_ext[id_ext_salida];
    nodo_entrada->conectarNodo(nodo_salida, peso);
}



struct elemento_cola{
    float peso;
    Nodo* nodo;
};

bool operator<(const elemento_cola& left, const elemento_cola& right)
{
    return (left.peso > right.peso);
}

vector<long> Grafo::computarCaminoMasCorto(long id_ext_nodo_inicio, long id_ext_nodo_final, long seccion){
    if(id_ext_nodo_inicio == id_ext_nodo_final){
        vector<long> emptyVector;
        return emptyVector;
    }

    priority_queue<elemento_cola> cola_pesos_a_evaluar;



    auto nodos_pendientes = new int[sigIdNodo];
    auto pesos = new float[sigIdNodo];
    auto nodo_anterior = new int[sigIdNodo];

    for(auto a: nodos_id_ext){
        Nodo* nodo = a.second;
        long id_ext = nodo->getIdExt();
        int id_int = nodo->getIdInt();

        if(id_ext == id_ext_nodo_inicio){
            nodos_pendientes[id_int] = 0;
            pesos[id_int] = 0;
        } else {
            nodos_pendientes[id_int] = 1;
            pesos[id_int] = -1;
        }

        nodo_anterior[id_int] = -1;
    }

    int id_int_inicio = id_ext_to_int[id_ext_nodo_inicio];
    int id_int_fin = id_ext_to_int[id_ext_nodo_final];

    int id_ultimo_nodo_agregado = id_int_inicio;

    while (id_ultimo_nodo_agregado != id_int_fin){

        Nodo* ultimo_nodo_revisado = nodos_id_int[id_ultimo_nodo_agregado];

        if(seccion == -1 || ultimo_nodo_revisado->getSeccion() == seccion){
            if(!ultimo_nodo_revisado->getNodosVecinos().empty()){
                for (auto nodo_peso_vecino: ultimo_nodo_revisado->getNodosVecinos()){
                    float peso_cantidado = pesos[id_ultimo_nodo_agregado] + nodo_peso_vecino.second;
                    float peso_actual_vecino = pesos[nodo_peso_vecino.first->getIdInt()];
                    if (peso_actual_vecino == -1 ||  peso_cantidado < peso_actual_vecino){
                        pesos[nodo_peso_vecino.first->getIdInt()] = peso_cantidado;
                        cola_pesos_a_evaluar.push({peso_cantidado, nodo_peso_vecino.first});
                        nodo_anterior[nodo_peso_vecino.first->getIdInt()] = id_ultimo_nodo_agregado;
                    }
                }
            }
        }

        bool encontrado = false;
        while(!encontrado && !cola_pesos_a_evaluar.empty()){
            elemento_cola elemento = cola_pesos_a_evaluar.top();
            cola_pesos_a_evaluar.pop();

            Nodo* nodo = elemento.nodo;

            if(nodos_pendientes[elemento.nodo->getIdInt()] == 1){ // El nodo esta pendiente
                nodos_pendientes[nodo->getIdInt()] = 0;
                id_ultimo_nodo_agregado = nodo->getIdInt();
                encontrado = true;
            }
        }

        if(cola_pesos_a_evaluar.empty() && !encontrado){
            vector<long> emptyVector;
            return emptyVector;
        }

    }

    vector<long> camino;
    camino.push_back(id_int_to_ext[id_ultimo_nodo_agregado]);
    int ultimo_nodo_camino = id_ultimo_nodo_agregado;
    while(ultimo_nodo_camino != id_int_inicio){
        ultimo_nodo_camino = nodo_anterior[ultimo_nodo_camino];
        camino.push_back(id_int_to_ext[ultimo_nodo_camino]);
    }

    reverse(camino.begin(), camino.end());

    delete[] nodos_pendientes;
    delete[] pesos;
    delete[] nodo_anterior;

    return camino;
}

vector<long> Grafo::computarCaminoMasCortoUtilizandoAStar(long id_ext_nodo_inicio, long id_ext_nodo_final){
    if(id_ext_nodo_inicio == id_ext_nodo_final){
        vector<long> emptyVector;
        return emptyVector;
    }

    priority_queue<elemento_cola> cola_pesos_a_evaluar;

    auto nodos_pendientes = new int[sigIdNodo];
    auto pesos = new float[sigIdNodo];
    auto nodo_anterior = new int[sigIdNodo];
    auto estimadores = new double[sigIdNodo];

    Nodo* nodoObjetivo = nodos_id_ext[id_ext_nodo_final];

    for(auto a: nodos_id_ext){
        Nodo* nodo = a.second;
        if(nodo != nullptr){
            long id_ext = nodo->getIdExt();
            int id_int = nodo->getIdInt();

            if(id_ext == id_ext_nodo_inicio){
                nodos_pendientes[id_int] = 0;
                pesos[id_int] = 0;
            } else {
                nodos_pendientes[id_int] = 1;
                pesos[id_int] = -1;
            }

            nodo_anterior[id_int] = -1;

            estimadores[id_int] = getDistanceEntrePuntosEnMetros(nodoObjetivo->getY(), nodoObjetivo->getX(), nodo->getY(), nodo->getX()) / 60.f;
        }
    }



    int id_int_inicio = id_ext_to_int[id_ext_nodo_inicio];
    int id_int_fin = id_ext_to_int[id_ext_nodo_final];

    int id_ultimo_nodo_agregado = id_int_inicio;

    while (id_ultimo_nodo_agregado != id_int_fin){

        Nodo* ultimo_nodo_revisado = nodos_id_int[id_ultimo_nodo_agregado];

        if(!ultimo_nodo_revisado->getNodosVecinos().empty()){
            for (auto nodo_peso_vecino: ultimo_nodo_revisado->getNodosVecinos()){
                float peso_cantidado = pesos[id_ultimo_nodo_agregado] + nodo_peso_vecino.second + estimadores[id_ultimo_nodo_agregado];
                float peso_actual_vecino = pesos[nodo_peso_vecino.first->getIdInt()];
                if (peso_actual_vecino == -1 ||  peso_cantidado < peso_actual_vecino){
                    pesos[nodo_peso_vecino.first->getIdInt()] = peso_cantidado;
                    cola_pesos_a_evaluar.push({peso_cantidado, nodo_peso_vecino.first});
                    nodo_anterior[nodo_peso_vecino.first->getIdInt()] = id_ultimo_nodo_agregado;
                }
            }
        }

        bool encontrado = false;
        while(!encontrado && !cola_pesos_a_evaluar.empty()){
            elemento_cola elemento = cola_pesos_a_evaluar.top();
            cola_pesos_a_evaluar.pop();

            Nodo* nodo = elemento.nodo;

            if(nodos_pendientes[elemento.nodo->getIdInt()] == 1){ // El nodo esta pendiente
                nodos_pendientes[nodo->getIdInt()] = 0;
                id_ultimo_nodo_agregado = nodo->getIdInt();
                encontrado = true;
            }
        }

        if(cola_pesos_a_evaluar.empty() && !encontrado){
            vector<long> emptyVector;
            return emptyVector;
        }

    }

    vector<long> camino;
    camino.push_back(id_int_to_ext[id_ultimo_nodo_agregado]);
    int ultimo_nodo_camino = id_ultimo_nodo_agregado;
    while(ultimo_nodo_camino != id_int_inicio){
        ultimo_nodo_camino = nodo_anterior[ultimo_nodo_camino];
        camino.push_back(id_int_to_ext[ultimo_nodo_camino]);
    }

    reverse(camino.begin(), camino.end());

    delete[] nodos_pendientes;
    delete[] pesos;
    delete[] nodo_anterior;
    delete[] estimadores;

    return camino;
}


Nodo* Grafo::obtenerNodo(long id_ext_nodo){
    return nodos_id_ext[id_ext_nodo];
}

Grafo::~Grafo() {
    for (auto n: nodos_id_ext){
        delete(n.second);
    }

}


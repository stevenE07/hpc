//
// Created by dpadron on 07/07/24.
//

#ifndef HPC_NODO_H
#define HPC_NODO_H

using namespace std;

class Nodo {
private:
    long id;
    vector<pair<Nodo*, float>> nodos_vecinos;
public:
    Nodo(long id){
        this->id = id;
    }
    void conectarNodo(Nodo* nodo, float peso){
        pair<Nodo*, float> arista;
        arista.first = nodo;
        arista.second = peso;
        nodos_vecinos.push_back(arista);
    }

    long getId() const {
        return id;
    }

    void setId(long id) {
        Nodo::id = id;
    }

    const vector<pair<Nodo *, float>> &getNodosVecinos() const {
        return nodos_vecinos;
    }

    void setNodosVecinos(const vector<pair<Nodo *, float>> &nodosVecinos) {
        nodos_vecinos = nodosVecinos;
    }
};


#endif //HPC_NODO_H

#ifndef HPC_NODO_H
#define HPC_NODO_H

using namespace std;

class Nodo {
private:
    int id_int;
    long id_ext;
    long seccion; //Identificador de la componente a la que pertenecen, en nuestro conexto es el barrio
    vector<pair<Nodo*, float>> nodos_vecinos;
public:
    Nodo(int id_int, long id_ext, long seccion){
        this->id_int = id_int;
        this->id_ext = id_ext;
        this->seccion = seccion;
    }
    void conectarNodo(Nodo* nodo, float peso){
        pair<Nodo*, float> arista;
        arista.first = nodo;
        arista.second = peso;
        nodos_vecinos.push_back(arista);
    }

    bool isConectadoCon(long id_nodo){
        for (auto nood_peso: nodos_vecinos){
            if (nood_peso.first->id_ext == id_nodo){
               return true;
            }
        }
        return false;
    }

    int getIdInt() const {
        return id_int;
    }

    void setIdInt(int idInt) {
        id_int = idInt;
    }

    long getIdExt() const {
        return id_ext;
    }

    void setIdExt(long idExt) {
        id_ext = idExt;
    }

    const vector<pair<Nodo *, float>> &getNodosVecinos() const {
        return nodos_vecinos;
    }

    void setNodosVecinos(const vector<pair<Nodo *, float>> &nodosVecinos) {
        nodos_vecinos = nodosVecinos;
    }

    long getSeccion() const {
        return seccion;
    }

    void setSeccion(long seccion) {
        Nodo::seccion = seccion;
    }
};


#endif //HPC_NODO_H

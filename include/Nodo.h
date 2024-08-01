#include "map"
#include "vector"
#include "omp.h"

#ifndef HPC_NODO_H
#define HPC_NODO_H

using namespace std;

class Nodo {
private:
    float x, y;
    int id_int;
    long id_ext;
    long seccion; //Identificador de la componente a la que pertenecen, en nuestro conexto es el barrio
    vector<pair<Nodo*, float>> nodos_vecinos;

    map<long, vector<long>> tablaDeRutas;

    omp_lock_t lock;

public:
    Nodo(int id_int, long id_ext, long seccion, float x, float y){
        this->id_int = id_int;
        this->id_ext = id_ext;
        this->seccion = seccion;
        this->x = x;
        this->y = y;
        omp_init_lock(&lock);
    }

    void consultarRutaPreCargada(long dest, vector<long> & ruta, bool & encontrada){
        omp_set_lock(&lock);
        encontrada = this->tablaDeRutas.find(dest) != tablaDeRutas.end();
        if(encontrada){
            ruta = tablaDeRutas[dest];
        }
        omp_unset_lock(&lock);
    }

    void agregarRutaPreCargada(long dest, vector<long>& ruta){

        if(ruta.empty()) return;

        omp_set_lock(&lock);

        if(this->tablaDeRutas.count(dest) == 0){
            tablaDeRutas[dest] = ruta;
        }
        omp_unset_lock(&lock);
    }

    void limpiarRutasPreCargadas(){
        omp_set_lock(&lock);

        for(auto lista: tablaDeRutas){
            lista.second.clear();
        }
        this->tablaDeRutas.clear();

        omp_unset_lock(&lock);
    }

    ~Nodo(){
        omp_destroy_lock(&lock);
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

    float getX() const {
        return x;
    }

    void setX(float x) {
        Nodo::x = x;
    }

    float getY() const {
        return y;
    }

    void setY(float y) {
        Nodo::y = y;
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

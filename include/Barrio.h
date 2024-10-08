#include "map"
#include "string"
#include "vector"
#include "set"

#ifndef HPC_BARRIO_H
#define HPC_BARRIO_H

using namespace std;


class Calle;
class Barrio {

private:

    long id;
    //string nombre;
    map<string, Calle*> mapa_calles;
    set<long> barriosVecinos;

    int numeroCallePoderadoPorNumeroCarrilesPerifericas = 0;

public:
    Barrio(long id);

    void agregarCalle(Calle*);

    bool isCalleEnBarrio(long id_src, long id_dst);

    void agregarBarrioVecino(long id_barrio){
        barriosVecinos.insert(id_barrio);
    }

    int getNumeroCallePoderadoPorNumeroCarrilesPerifericas() const;

    void setNumeroCallePoderadoPorNumeroCarrilesPerifericas(int numeroCallePoderadoPorNumeroCarrilesPerifericas);

    const set<long> &getBarriosVecinos() const;

    void setBarriosVecinos(const set<long> &barriosVecinos);

    Calle* obtenerCalle(string & idCalle);

    long getId() const;

    void setId(long id);

    void addCalles(vector<Calle*> & calles);

    ~Barrio();
};
#include "Calle.h"

#endif


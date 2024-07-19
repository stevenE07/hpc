#include "../include/Barrio.h"

Barrio::Barrio(long id){
    this->id = id;
}

void Barrio::agregarCalle(Calle* calle){
    this->mapa_calles[Calle::getIdCalle(calle)] = calle;
}

bool Barrio::isCalleEnBarrio(long id_src, long id_dst){
    return mapa_calles.find(Calle::getIdCalle(id_src, id_dst)) != mapa_calles.end();
}

Calle* Barrio::obtenerCalle(string & idCalle){
    return mapa_calles[idCalle];
}

long Barrio::getId() const {
    return id;
}

void Barrio::setId(long id) {
    Barrio::id = id;
}

void Barrio::addCalles(vector<Calle*> & calles){
    for (auto c: mapa_calles){
        calles.push_back(c.second);
    }
}

Barrio::~Barrio(){
    for(auto m : mapa_calles){
        delete m.second;
    }
}

//
// Created by daniel on 22/06/24.
//

#include "../include/Vehiculo.h"

Vehiculo::Vehiculo(int id,unsigned long epoca_inicial,float velocidad ){
    this->id = id;
    this-> epoca_inicial = epoca_inicial;
    this->velocidad = velocidad;
    this->indice_calle_recorrida = 0;
    this->esperando_traslado_entre_calles = false;
}

unsigned int Vehiculo::getId() const {
    return id;
}

void Vehiculo::setId(unsigned int id) {
    Vehiculo::id = id;
}

unsigned long Vehiculo::getEpocaInicial() const {
    return epoca_inicial;
}

void Vehiculo::setEpocaInicial(unsigned long epocaInicial) {
    epoca_inicial = epocaInicial;
}

unsigned long Vehiculo::getEpocaFinal() const {
    return epoca_final;
}

void Vehiculo::setEpocaFinal(unsigned long epocaFinal) {
    epoca_final = epocaFinal;
}



float Vehiculo::getVelocidad() const {
    return velocidad;
}

void Vehiculo::setVelocidad(float velocidad) {
    Vehiculo::velocidad = velocidad;
}

unsigned int Vehiculo::getNumeroCalleRecorrida() const {
    return indice_calle_recorrida;
}

void Vehiculo::setNumeroCalleRecorrida(unsigned int numeroCalleRecorrida) {
    indice_calle_recorrida = numeroCalleRecorrida;
}

string Vehiculo::sigCalleARecorrer(){
    indice_calle_recorrida ++;
    return to_string(ruta[indice_calle_recorrida]) + "-" +  to_string(ruta[indice_calle_recorrida + 1]);
}

bool Vehiculo::isEsperandoTrasladoEntreCalles() const {
    return esperando_traslado_entre_calles;
}

void Vehiculo::setEsperandoTrasladoEntreCalles(bool esperandoTrasladoEntreCalles) {
    esperando_traslado_entre_calles = esperandoTrasladoEntreCalles;
}

const vector<long> &Vehiculo::getRuta() const {
    return ruta;
}

void Vehiculo::setRuta(const vector<long> &ruta) {
    Vehiculo::ruta = ruta;
}



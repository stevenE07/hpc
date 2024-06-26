//
// Created by daniel on 22/06/24.
//

#include "../include/Vehiculo.h"

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

const std::vector<unsigned int> &Vehiculo::getRuta() const {
    return ruta;
}

void Vehiculo::setRuta(const std::vector<unsigned int> &ruta) {
    Vehiculo::ruta = ruta;
}

float Vehiculo::getVelocidad() const {
    return velocidad;
}

void Vehiculo::setVelocidad(float velocidad) {
    Vehiculo::velocidad = velocidad;
}

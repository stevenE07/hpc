//
// Created by daniel on 22/06/24.
//

#include "../include/Vehiculo.h"

Vehiculo::Vehiculo(int id){
    this->id = id;
    this->indice_calle_recorrida = 0;
    this->is_segmento_final = 0;
    this->esperando_traslado_entre_calles = false;
    this->calleactual = nullptr;
}

int Vehiculo::getId() const {
    return id;
}

void Vehiculo::setId(unsigned int id) {
    Vehiculo::id = id;
}
unsigned char Vehiculo::get_is_segmento_final() {
    return this->is_segmento_final;
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

long Vehiculo::sigNodoARecorrer(){
    indice_calle_recorrida ++;
    return ruta[indice_calle_recorrida + 1];
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

void Vehiculo::setRuta(const vector<long> &ruta, unsigned char is_segmento_final) {
    this->ruta = ruta;
    this->is_segmento_final = is_segmento_final;
}


void Vehiculo::set_indice_calle_recorrida(int valor){
    Vehiculo::indice_calle_recorrida = valor;
}

void Vehiculo::imprimirRuta() {
        std::cout << "id_vehiculo: " << this->getId() << std::endl;
    for (const auto & r: ruta) {
        std::cout << r << " |  ";
    }
    std::cout << std::endl;

}

long Vehiculo::nodo_destino(){
    return ruta.back();
}

long Vehiculo::nodo_actual() {
    return ruta[indice_calle_recorrida];
}

Calle *Vehiculo::getCalleactual() const {
    return calleactual;
}

void Vehiculo::setCalleactual(Calle *calleactual) {
    Vehiculo::calleactual = calleactual;
}

string Vehiculo::getIdCalle(){
     return to_string(ruta[indice_calle_recorrida]) + "-" + to_string(ruta[indice_calle_recorrida + 1]);
}


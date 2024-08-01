//
// Created by daniel on 22/06/24.
//

#include "../include/Vehiculo.h"

Vehiculo::Vehiculo(int id, long barrio_inicial){
    this->id = id;
    this->indice_calle_recorrida = 0;
    this->is_segmento_final = 0;
    this->esperando_notificacion = false;
    this->calleactual = nullptr;
    this->distancia_recorrida = 0.0f;
    this->id_barrio_inicio = barrio_inicial;

    omp_init_lock(&lock_esperando_notificacion);
}

Vehiculo::~Vehiculo() {
    omp_destroy_lock(&lock_esperando_notificacion);
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

float Vehiculo::getDistanciaRecorrida() const {
    return distancia_recorrida;
}

void Vehiculo::setDistanciaRecorrida(float distanciaRecorrida) {
    distancia_recorrida = distanciaRecorrida;
}

int Vehiculo::getEpocaInicio() const {
    return epoca_inicio;
}

void Vehiculo::setEpocaInicio(int epocaInicio) {
    epoca_inicio = epocaInicio;
}

bool Vehiculo::isEsperandoNotificacion() const {
    return esperando_notificacion;
}

void Vehiculo::setEsperandoNotificacion(bool esperandoNotificacion) {
    esperando_notificacion = esperandoNotificacion;
}

long Vehiculo::getIdBarrioInicio() const {
    return id_barrio_inicio;
}

void Vehiculo::setIdBarrioInicio(long idBarrioInicio) {
    id_barrio_inicio = idBarrioInicio;
}



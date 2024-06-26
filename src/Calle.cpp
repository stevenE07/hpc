#include "../include/Calle.h"
#include "cmath"
#include "iostream"

Calle::Calle(unsigned int id_nodo_inicial, unsigned int id_nodo_final, float largo, unsigned numero_carriles,float velocidad_maxima){
   this->nodo_inicial = id_nodo_inicial;
   this->nodo_final = id_nodo_final;
   this->largo = largo;
   this->numero_carriles = numero_carriles;
   this->velocidad_maxima = velocidad_maxima;
}

void Calle::insertarSolicitudTranspaso(Calle* calleSolicitante, Vehiculo* vehiculo){
    pair<Calle*, Vehiculo*> solicitud;
    solicitud.first = calleSolicitante;
    solicitud.second = vehiculo;

    solicitudes_traspaso_calle.push(solicitud);
}

void Calle::notificarTranspasoCompleto(unsigned int idVehiculo){
    notificaciones_traslado_calle_realizado.insert(idVehiculo);
}

// inicio carril 1 0---|---|----|--->TOPE
// inicio carril 2 0--|---|----|--->TOPE
void Calle::ejecutarEpoca(float tiempo_epoca) {

    /*
     * 1. Remover vehiculos aceptados por otras calles
     * 2. Actualizar vehiculos en calle
     * 3. Aceptar vehiculos solicitantes
     * 4. Solicitar transpaso
     */

    // 2- Actualizar vehiculo en calle
    auto maximoPorCarril = new float[numero_carriles];

    for (int i = 0; i < numero_carriles; i++) {
        maximoPorCarril[i] = this->largo;
    }

    vector<Vehiculo *> vehiculos_ordenados_en_calle_aux;

    for (Vehiculo *v: vehculos_ordenados_en_calle) {
        // Actualizo velocidad vehiculo
        v->setVelocidad(velocidad_maxima); //ToDo aumentar complejidad

        // Calculamos el movimiento ideal
        float velocidad = v->getVelocidad();
        float desplasamiento_ideal = velocidad / (3.6f) * (tiempo_epoca / 1000.f);

        // Desplazamos el vehiculo
        auto carrilPosicion = posiciones_vehiculos_en_calle[v->getId()];
        int numeroCarril = carrilPosicion.first;
        float posicion = carrilPosicion.second;

        float topeEnCarril = maximoPorCarril[numeroCarril];

        pair<int, float> nuevaPosicion;
        nuevaPosicion.first = numeroCarril;
        nuevaPosicion.second = fmin(topeEnCarril, posicion + desplasamiento_ideal);

        v->setVelocidad( (float)nuevaPosicion.second * (3.6f) / (tiempo_epoca / 1000.f));

        posiciones_vehiculos_en_calle[v->getId()] = nuevaPosicion;

        maximoPorCarril[numeroCarril] = nuevaPosicion.second - LARGO_VEHICULO;

        vehiculos_ordenados_en_calle_aux.push_back(v);
    }

    vehculos_ordenados_en_calle = vehiculos_ordenados_en_calle_aux;

    // 3- aceptar vehiculo solicitante, en principio lo hacemos naive aceptando el primero de la cola.
    for (int num_carril = 0; num_carril < numero_carriles; num_carril++) {
        if (maximoPorCarril[num_carril] - LARGO_VEHICULO > 0){
            pair<Calle*, Vehiculo*> solicitud = solicitudes_traspaso_calle.front();
            solicitudes_traspaso_calle.pop();


            pair<int, float> carrilPosicion;
            carrilPosicion.first = num_carril;
            carrilPosicion.second = 0.f;
            posiciones_vehiculos_en_calle[solicitud.second->getId()] = carrilPosicion;

            vehculos_ordenados_en_calle.push_back(solicitud.second);

            solicitud.first->notificarTranspasoCompleto(solicitud.second->getId());
        }
    }

    delete maximoPorCarril;
}

void Calle::mostrarEstado(){
    string idCalle = Calle::getIdCalle(this);

    cout << " >>>>>>>>>>>  Info calle: " << idCalle << endl;

    for( Vehiculo* v: vehculos_ordenados_en_calle){
        auto carrilPosicion = posiciones_vehiculos_en_calle[v->getId()];

        cout << " id = " << v->getId() << " | carril = " << carrilPosicion.first
        << " | posicion= " << carrilPosicion.second  << " | velocidad = " << v->getVelocidad() << endl;
    }

}
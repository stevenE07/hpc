#include <functional>
#include "../include/Calle.h"
#include "cmath"
#include "iostream"

Calle::Calle(long id_nodo_inicial, long id_nodo_final, float largo, unsigned numero_carriles, float velocidad_maxima, function<Calle*(string)> & obtenerCallePorIdFn, function<void()>& doneFn){
   this->nodo_inicial = id_nodo_inicial;
   this->nodo_final = id_nodo_final;
   this->largo = largo;
   this->numero_carriles = numero_carriles;
   this->velocidad_maxima = velocidad_maxima;
   this->obtenerCallePorIdFn = obtenerCallePorIdFn;
   this->doneFn = doneFn;
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

        //eliminamos los vehiculos que fueron aceptados.
        if( notificaciones_traslado_calle_realizado.find(v->getId()) != notificaciones_traslado_calle_realizado.end()) {
            notificaciones_traslado_calle_realizado.erase(v->getId());
            posiciones_vehiculos_en_calle.erase(v->getId());
            continue;
        }

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

        float nuevaPosicion = fmin(topeEnCarril, posicion + desplasamiento_ideal);

        pair<int, float> nuevaCarilPosicion;
        nuevaCarilPosicion.first = numeroCarril;
        nuevaCarilPosicion.second = nuevaPosicion;

        if(nuevaCarilPosicion.second >= largo){
           if(!v->isEsperandoTrasladoEntreCalles()){
               if(v->getNumeroCalleRecorrida() + 1 == v->getRuta().size() - 1){
                  LOG(INFO) << " #####################################  Vehiculo con ID: " << v->getId() << " Termino";
                  doneFn();
                  posiciones_vehiculos_en_calle.erase(v->getId());
                  continue;
               } else {
                  v->setEsperandoTrasladoEntreCalles(true);
                  Calle* sigCalle = obtenerCallePorIdFn(v->sigCalleARecorrer());
                  sigCalle->insertarSolicitudTranspaso(this, v);
               }
           }
        }

        v->setVelocidad( ((float)nuevaPosicion - posicion) * (3.6f) / (tiempo_epoca / 1000.f));

        posiciones_vehiculos_en_calle[v->getId()] = nuevaCarilPosicion;

        maximoPorCarril[numeroCarril] = nuevaPosicion - LARGO_VEHICULO;

        vehiculos_ordenados_en_calle_aux.push_back(v);
    }

    vehculos_ordenados_en_calle = vehiculos_ordenados_en_calle_aux;

    // 3- aceptar vehiculo solicitante, en principio lo hacemos naive aceptando el primero de la cola.
    for (int num_carril = 0; num_carril < numero_carriles; num_carril++) {
        if (maximoPorCarril[num_carril] - LARGO_VEHICULO > 0){
            if(solicitudes_traspaso_calle.size() > 0){
                pair<Calle*, Vehiculo*> solicitud = solicitudes_traspaso_calle.front();
                solicitudes_traspaso_calle.pop();


                pair<int, float> carrilPosicion;
                carrilPosicion.first = num_carril;
                carrilPosicion.second = 0.f;
                posiciones_vehiculos_en_calle[solicitud.second->getId()] = carrilPosicion;

                vehculos_ordenados_en_calle.push_back(solicitud.second);

                solicitud.second->setEsperandoTrasladoEntreCalles(false);

                if(solicitud.first != nullptr){
                    solicitud.first->notificarTranspasoCompleto(solicitud.second->getId());
                }
            }
        }
    }

    delete maximoPorCarril;
}

void Calle::mostrarEstado(){

    if(vehculos_ordenados_en_calle.empty()){
        return;
    }

    string idCalle = Calle::getIdCalle(this);

    LOG(INFO) << " >>>>>>>>>>>  Info calle: " << idCalle;


    for( Vehiculo* v: vehculos_ordenados_en_calle){
        auto carrilPosicion = posiciones_vehiculos_en_calle[v->getId()];

        LOG(INFO) << " id = " << v->getId() << " | carril = " << carrilPosicion.first
        << " | posicion= " << carrilPosicion.second  << " | velocidad = " << v->getVelocidad() << "Km";
    }

}
#include <functional>
#include "../include/Calle.h"
#include "cmath"
#include "iostream"
#include <mpi.h>

Calle::Calle(long id_nodo_inicial, long id_nodo_final, float largo, unsigned numero_carriles, float velocidad_maxima,
             map<long, Barrio*> & mapa_barrio,
             function<void()>& doneFn,
             function<void(SolicitudTranspaso&)>& enviarSolicitudFn,
             Grafo* grafo,map<long, int>& asignacion_barrios ){
   this->nodo_inicial = id_nodo_inicial;
   this->nodo_final = id_nodo_final;
   this->largo = largo;
   this->numero_carriles = numero_carriles;
   this->velocidad_maxima = velocidad_maxima;
   this->mapa_barrio = mapa_barrio;
   this->doneFn = doneFn;
   this->enviarSolicitudFn = enviarSolicitudFn;
   this->grafo = grafo;
   this->asignacion_barrios = asignacion_barrios;
   omp_init_lock(&lock_solicitud);
   omp_init_lock(&lock_notificacion);
}

void Calle::insertarSolicitudTranspaso(Calle* calleSolicitante, Vehiculo* vehiculo){
    pair<Calle*, Vehiculo*> solicitud;
    solicitud.first = calleSolicitante;
    solicitud.second = vehiculo;

    omp_set_lock(&lock_solicitud); //Mutex al insertar solicutud
    solicitudes_traspaso_calle.push_back(solicitud);
    omp_unset_lock(&lock_solicitud);
}

void Calle::notificarTranspasoCompleto(unsigned int idVehiculo){
    omp_set_lock(&lock_notificacion); //Mutex al insertar notificacion
    notificaciones_traslado_calle_realizado.insert(idVehiculo);
    omp_unset_lock(&lock_notificacion);
}

// inicio carril 1 0---|---|----|--->TOPE
// inicio carril 2 0--|---|----|--->TOPE
void Calle::ejecutarEpoca(float tiempo_epoca) {
    /*
     * 1 y 2. Remover vehiculos aceptados por otras calles
     * 1 y 2. Actualizar vehiculos en calle
     * 3. Aceptar vehiculos solicitantes
     * 4. Solicitar transpaso
     */

    // 1 y 2- Remover vehiculos aceptados por otras calles y Actualizar vehiculos en calle


    auto maximoPorCarril = new float[numero_carriles];

    for (int i = 0; i < numero_carriles; i++) {
        maximoPorCarril[i] = this->largo;
    }

    vector<Vehiculo *> vehiculos_ordenados_en_calle_aux;

    for (Vehiculo *v: vehculos_ordenados_en_calle) {

        omp_set_lock(&lock_notificacion);
        //eliminamos los vehiculos que fueron aceptados.
        if( notificaciones_traslado_calle_realizado.find(v->getId()) != notificaciones_traslado_calle_realizado.end()) {
            notificaciones_traslado_calle_realizado.erase(v->getId());
            omp_unset_lock(&lock_notificacion);

            posiciones_vehiculos_en_calle.erase(v->getId());
            continue;
        }
        omp_unset_lock(&lock_notificacion);

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
           if(!v->isEsperandoTrasladoEntreCalles()) {
               if(v->getNumeroCalleRecorrida() + 1 == v->getRuta().size() - 1) {
                   // termino de computar la ultima calle dentro del barrio.
                   #pragma omp critical
                   LOG(INFO) << " #####################################  Vehiculo con ID: " << v->getId() << " Termino_barrio";
                   if(v->get_is_segmento_final()) { // si el segmento es final.
                       //si es la ultima el ultimo segmento por transitar.
                       doneFn();
                       posiciones_vehiculos_en_calle.erase(v->getId());
                       continue;
                   }else { // tengo que cambiar de barrio.
                       long idBarrioActualCalle = grafo->obtenerNodo(nodo_inicial)->getSeccion();
                       long idBarrioSig = grafo->obtenerNodo(nodo_final)->getSeccion();
                       long idSiguienteNodo = v->sigNodoARecorrer();
                       string codigoSiguienteCalle = Calle::getIdCalle(nodo_final, idSiguienteNodo);
                       if (asignacion_barrios[idBarrioActualCalle] == asignacion_barrios[idBarrioSig]) { // si el barrio es del mismo nodo mpi.
                           Calle *sigCalle = mapa_barrio[idBarrioSig]->obtenerCalle(codigoSiguienteCalle);
                           sigCalle->insertarSolicitudTranspaso(this, v);
                       }else {
                           SolicitudTranspaso solicitudTranspaso;
                           solicitudTranspaso.id_vehiculo = v->getId();
                           solicitudTranspaso.id_barrio = idBarrioSig;
                           solicitudTranspaso.id_nodo_inicial_calle_anterior = nodo_inicial;

                           #pragma omp critical
                           this->enviarSolicitudFn(solicitudTranspaso);

                           printf("SOLICITUD!!!!");
                           exit(1);
                       }
                   }
               }else { // continua computando calles dentro del mismo barrio.
                   v->setEsperandoTrasladoEntreCalles(true);
                   long idBarrio = grafo->obtenerNodo(nodo_final)->getSeccion();
                   long idSiguienteNodo = v->sigNodoARecorrer();
                   string codigoSiguienteCalle = Calle::getIdCalle(nodo_final, idSiguienteNodo);
                   //si el barrio pertenece al mismo barrio que el nodo actual entoneces lo gestiona el mismo nodo mpi.
                   if (asignacion_barrios[idBarrioActualCalle] ==
                       asignacion_barrios[idBarrioSigCalle]) { //Si el nodo MPI encargado de la solictud es el mismo
                       Calle *sigCalle = mapa_barrio[idBarrioSigCalle]->obtenerCalle(codigoSiguienteCalle);
                       sigCalle->insertarSolicitudTranspaso(this, v);
                   } else {
                       //aca se envia la informacion desde el nodo mpi del barrio actual al siguiente.

                       SolicitudTranspaso solicitudTranspaso;
                       solicitudTranspaso.id_vehiculo = v->getId();
                       solicitudTranspaso.id_barrio = idBarrioSigCalle;
                       solicitudTranspaso.id_nodo_inicial_calle_anterior = nodo_inicial;

                       #pragma omp critical
                       this->enviarSolicitudFn(solicitudTranspaso);

                       printf("SOLICITUD!!!!");
                       //exit(1);

                   }
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
    omp_set_lock(&lock_solicitud);
    /*
    long data_received[4];
    MPI_Request request;
    MPI_Irecv(data_received, 4, MPI_LONG, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &request);
    MPI_Status status;
    int flag;
    MPI_Test(&request, &flag, &status);
     */
    for (int num_carril = 0; num_carril < numero_carriles; num_carril++) {
        if (maximoPorCarril[num_carril] - LARGO_VEHICULO > 0){
            if(!solicitudes_traspaso_calle.empty()){

                int posicion_sig_vehiculo = -1;

                int contador = 0;
                for(auto calle_vehiculo: solicitudes_traspaso_calle){
                    if(posicion_sig_vehiculo == -1 ){
                        posicion_sig_vehiculo = contador;
                    } else {
                        if(solicitudes_traspaso_calle[posicion_sig_vehiculo].first == nullptr
                        && calle_vehiculo.first != nullptr){
                            posicion_sig_vehiculo = contador;
                        }
                    }
                    contador++;
                }


                pair<Calle*, Vehiculo*> solicitud = solicitudes_traspaso_calle[posicion_sig_vehiculo];
                solicitudes_traspaso_calle.erase(solicitudes_traspaso_calle.begin() + posicion_sig_vehiculo);

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
    omp_unset_lock(&lock_solicitud);

    delete[] maximoPorCarril;
}

void Calle::mostrarEstado(){

    if(vehculos_ordenados_en_calle.empty() && solicitudes_traspaso_calle.empty()){
        return;
    }

    string idCalle = Calle::getIdCalle(this);

    LOG(INFO) << " >>>>>>>>>>>  Info calle: " << idCalle << " cantidad_carriles:" << this->numero_carriles << "largo: " << this->largo ;


    for( Vehiculo* v: vehculos_ordenados_en_calle){
        auto carrilPosicion = posiciones_vehiculos_en_calle[v->getId()];

        LOG(INFO) << " id = " << v->getId() << " | carril = " << carrilPosicion.first
        << " | posicion= " << carrilPosicion.second << " tope:" << this->largo << " | velocidad = " << v->getVelocidad() << "Km";
    }

    LOG(INFO) << " >>>>>>>>>>>  SOLICITUDES DE AUTOS CALLE:  " << getIdCalle(this) << " : " <<  this->solicitudes_traspaso_calle.size();

}

Calle::~Calle(){
    omp_destroy_lock(&lock_solicitud);
    omp_destroy_lock(&lock_notificacion);
}
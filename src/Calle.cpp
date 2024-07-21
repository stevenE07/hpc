#include <functional>
#include "../include/Calle.h"
#include "cmath"
#include "iostream"
#include <mpi.h>

Calle::Calle(long id_nodo_inicial, long id_nodo_final, float largo, unsigned numero_carriles, float velocidad_maxima,
             map<long, Barrio*> & mapa_barrio,
             function<void()>& doneFn,
             function<void(SolicitudTranspaso&)>& enviarSolicitudFn,
             function<void(NotificacionTranspaso &)>& enviarNotificacionFn,
             Grafo* grafo,
             map<long, int>& asignacion_barrios,
             map<pair<int, long>, queue<SegmentoTrayectoVehculoEnBarrio>> * ptr_segmentos_a_recorrer_por_barrio_por_vehiculo){
   this->nodo_inicial = id_nodo_inicial;
   this->nodo_final = id_nodo_final;
   this->largo = largo;
   this->numero_carriles = numero_carriles;
   this->velocidad_maxima = velocidad_maxima;
   this->mapa_barrio = mapa_barrio;
   this->doneFn = doneFn;
   this->enviarSolicitudFn = enviarSolicitudFn;
   this->enviarNotificacionFn = enviarNotificacionFn;
   this->grafo = grafo;
   this->asignacion_barrios = asignacion_barrios;
   this->ptr_segmentos_a_recorrer_por_barrio_por_vehiculo = ptr_segmentos_a_recorrer_por_barrio_por_vehiculo;
   omp_init_lock(&lock_solicitud);
   omp_init_lock(&lock_notificacion);
}

void Calle::insertarSolicitudTranspaso(long id_inicio_calle_solicitante, long id_fin_calle_solicitante, Vehiculo* vehiculo){
    pair<pair<long, long>, Vehiculo*> solicitud;
    solicitud.first = make_pair(id_inicio_calle_solicitante, id_fin_calle_solicitante);
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



bool isCalleNula(pair<long, long> idCalle){
    return idCalle.first == -1 && idCalle.second == -1;
}


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

            v->setEsperandoTrasladoEntreCalles(false);

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
                   if(v->get_is_segmento_final() == 1) { // si el segmento es final.
                       //si es la ultima el ultimo segmento por transitar.
                       #pragma omp critical
                       doneFn();
                       posiciones_vehiculos_en_calle.erase(v->getId());
                       continue;
                   } else { // tengo que cambiar de barrio.
                       long idBarrioActualCalle = grafo->obtenerNodo(nodo_inicial)->getSeccion();
                       long idBarrioSig = grafo->obtenerNodo(nodo_final)->getSeccion();
                       if (asignacion_barrios[idBarrioActualCalle] == asignacion_barrios[idBarrioSig]) { // si el barrio es del mismo nodo mpi.

                           pair<int, long> clave = make_pair(v->getId(), idBarrioSig);
                           SegmentoTrayectoVehculoEnBarrio sigSegmento = (*ptr_segmentos_a_recorrer_por_barrio_por_vehiculo)[clave].front();

                           #pragma omp critical
                           (*ptr_segmentos_a_recorrer_por_barrio_por_vehiculo)[clave].pop();

                           auto caminoSigBarrio = grafo->computarCaminoMasCorto(sigSegmento.id_inicio, sigSegmento.id_fin); //ToDo mejorar que solo busque en el barrio
                           v->setRuta(caminoSigBarrio, sigSegmento.is_segmento_final);
                           v->set_indice_calle_recorrida(0);

                           string idSigCalle = Calle::getIdCalle(caminoSigBarrio[0], caminoSigBarrio[1]);
                           Calle *sigCalle = mapa_barrio[idBarrioSig]->obtenerCalle(idSigCalle);
                           v->setEsperandoTrasladoEntreCalles(true);


                           sigCalle->insertarSolicitudTranspaso(nodo_inicial, nodo_final, v);
                       }else {
                           SolicitudTranspaso solicitudTranspaso;
                           solicitudTranspaso.id_vehiculo = v->getId();
                           solicitudTranspaso.id_barrio = idBarrioSig;
                           solicitudTranspaso.id_nodo_inicial_calle_anterior = nodo_inicial;

                           v->setEsperandoTrasladoEntreCalles(true);

                           #pragma omp critical
                           this->enviarSolicitudFn(solicitudTranspaso);

                           printf("SOLICITUD ENVIADA!!!!\n");
                       }
                   }
               }else { // continua computando calles dentro del mismo barrio.
                   v->setEsperandoTrasladoEntreCalles(true);
                   long idBarrio = grafo->obtenerNodo(nodo_final)->getSeccion();
                   long idSiguienteNodo = v->sigNodoARecorrer();
                   string codigoSiguienteCalle = Calle::getIdCalle(nodo_final, idSiguienteNodo);
                   Calle *sigCalle = mapa_barrio[idBarrio]->obtenerCalle(codigoSiguienteCalle);

                   sigCalle->insertarSolicitudTranspaso(nodo_inicial, nodo_final, v);
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


    /*
    long data_received[4];
    MPI_Request request;
    MPI_Irecv(data_received, 4, MPI_LONG, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &request);
    MPI_Status status;
    int flag;
    MPI_Test(&request, &flag, &status);
     */

    omp_set_lock(&lock_solicitud);

    for (int num_carril = 0; num_carril < numero_carriles; num_carril++) {
        if (maximoPorCarril[num_carril] - LARGO_VEHICULO > 0){
            if(!solicitudes_traspaso_calle.empty()){


                // Logica de cual vehiculo elegir de otra calle

                int indice_sig_solicitud_aceptada = -1;

                int contador = 0;
                for(auto calle_vehiculo: solicitudes_traspaso_calle){



                    if(indice_sig_solicitud_aceptada == -1 ){
                        indice_sig_solicitud_aceptada = contador;
                    } else {
                        if(isCalleNula(solicitudes_traspaso_calle[indice_sig_solicitud_aceptada].first)
                        && !isCalleNula(calle_vehiculo.first)){
                            indice_sig_solicitud_aceptada = contador;
                        }
                    }
                    contador++;
                }


                auto solicitud = solicitudes_traspaso_calle[indice_sig_solicitud_aceptada];
                solicitudes_traspaso_calle.erase(solicitudes_traspaso_calle.begin() + indice_sig_solicitud_aceptada);

                Vehiculo* vehiculoIngresado = solicitud.second;

                pair<int, float> carrilPosicion;
                carrilPosicion.first = num_carril;
                carrilPosicion.second = 0.f;

                posiciones_vehiculos_en_calle[vehiculoIngresado->getId()] = carrilPosicion;

                vehculos_ordenados_en_calle.push_back(vehiculoIngresado);

                if(!isCalleNula(solicitud.first)){

                    long idNodoInicialNotificante = solicitud.first.first;
                    long idNodoFinalNotificante = solicitud.first.second;

                    long idBarrioCalleANotificar = grafo->obtenerNodo(idNodoInicialNotificante)->getSeccion();

                    long idBarrioActual = grafo->obtenerNodo(nodo_inicial)->getSeccion();

                    if(asignacion_barrios[idBarrioCalleANotificar] != asignacion_barrios[idBarrioActual]){

                        NotificacionTranspaso notificacion;
                        notificacion.id_vehiculo = vehiculoIngresado->getId();

                        #pragma omp critical
                        enviarNotificacionFn(notificacion);

                        printf("NOTIFICACION EXTERNA \n");

                    } else {
                        string idCalleANotificar = Calle::getIdCalle(idNodoInicialNotificante, idNodoFinalNotificante);
                        Calle* calleANotificar = mapa_barrio[idBarrioCalleANotificar]->obtenerCalle(idCalleANotificar);
                        calleANotificar->notificarTranspasoCompleto(solicitud.second->getId());
                    }


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
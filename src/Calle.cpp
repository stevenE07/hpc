#include <functional>
#include "../include/Calle.h"
#include "cmath"
#include "iostream"
#include <mpi.h>

Calle::Calle(long id_nodo_inicial, long id_nodo_final, float largo, unsigned numero_carriles, float velocidad_maxima,
             map<long, Barrio*> & mapa_barrio,
             function<void(float, int)>& doneFn,
             function<void(SolicitudTranspaso&)>& enviarSolicitudFn,
             function<void(NotificacionTranspaso &)>& enviarNotificacionFn,
             Grafo* grafo,
             map<long, int>& asignacion_barrios,
             map<pair<int, long>, queue<SegmentoTrayectoVehculoEnBarrio>> * ptr_segmentos_a_recorrer_por_barrio_por_vehiculo,
             int my_rank){
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

   this->my_rank = my_rank;

   omp_init_lock(&lock_solicitud);
   omp_init_lock(&lock_notificacion);
}

float Calle::calcularCongestion(){

    int cantidad_max_vehiculos = floor((largo / LARGO_VEHICULO));
    int cantidad_vehiculos = (int)vehculos_ordenados_en_calle.size();

    return (float) cantidad_vehiculos / (float)cantidad_max_vehiculos;
}

float Calle::calcularVelocidadMedia(vector<float> & velocidadesVehiculos){
    float acumulado = 0.f;

    int cantidad = (int)velocidadesVehiculos.size();

    for(auto v: velocidadesVehiculos){
        acumulado += v;
    }

    float velocidadMedia;
    if(cantidad == 0){
        cantidad = 1; //Asi se puede hacer la divicion
    }


    return acumulado / (float)cantidad;

}




void Calle::insertarSolicitudTranspaso(long id_inicio_calle_solicitante, long id_fin_calle_solicitante, Vehiculo* vehiculo){
    pair<pair<long, long>, Vehiculo*> solicitud;
    solicitud.first = make_pair(id_inicio_calle_solicitante, id_fin_calle_solicitante);
    solicitud.second = vehiculo;

    omp_set_lock(&lock_solicitud); //Mutex al insertar solicutud
    solicitudes_traspaso_calle.push_back(solicitud);
    omp_unset_lock(&lock_solicitud);
}

void Calle::notificarTranspasoCompleto(int idVehiculo, bool eliminar_luego_de_notificar){
    omp_set_lock(&lock_notificacion); //Mutex al insertar notificacion
    notificaciones_traslado_calle_realizado.insert(make_pair(idVehiculo, eliminar_luego_de_notificar));
    omp_unset_lock(&lock_notificacion);
}



bool isCalleNula(pair<long, long> idCalle){
    return idCalle.first == -1 && idCalle.second == -1;
}


void Calle::ejecutarEpoca(float tiempo_epoca, int numeroEpoca) {
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

    vector<float> velocidadesVehiculos;

    vector<Vehiculo *> vehiculos_ordenados_en_calle_aux;

    for (Vehiculo *v: vehculos_ordenados_en_calle) {


        omp_set_lock(&lock_notificacion);
        //eliminamos los vehiculos que fueron aceptados.

        bool hay_notificacion = false;
        pair<int, bool> mi_notificacion;
        for (auto notificacion : notificaciones_traslado_calle_realizado ){
            if(notificacion.first == v->getId()){
                hay_notificacion = true;
                mi_notificacion = notificacion;
                break;
            }
        }

        if( hay_notificacion) {
            notificaciones_traslado_calle_realizado.erase(mi_notificacion);
            omp_unset_lock(&lock_notificacion);

            v->setEsperandoTrasladoEntreCalles(false);
            v->setContadorDePasienciaActivado(false);
            v->setNumeroEpocasAntesDeCambio(0);
            v->setNumeroIntentosCambioDeRuta(0);
            posiciones_vehiculos_en_calle.erase(v->getId());

            bool vehiculo_insertado_en_otro_nodo_mpi = mi_notificacion.second;

            if(vehiculo_insertado_en_otro_nodo_mpi ){
                delete v;
            }

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

               v->setNumeroEpocasAntesDeCambio(NUMERO_EPOCAS_ANTE_DE_REACALCULO_DE_RUTAS);
               v->setNumeroIntentosCambioDeRuta(0);

               if(v->getNumeroCalleRecorrida() + 1 == v->getRuta().size() - 1) {
                   if(v->get_is_segmento_final() == 1) { // si el segmento es final.
                       //si es la ultima el ultimo segmento por transitar.
                       #pragma omp critical(doneFn_mutex)
                       doneFn(v->getDistanciaRecorrida(), numeroEpoca - v->getEpocaInicio());
                       posiciones_vehiculos_en_calle.erase(v->getId());
                       continue;
                   } else { // tengo que cambiar de barrio.
                       long idBarrioActualCalle = grafo->obtenerNodo(nodo_inicial)->getSeccion();
                       long idBarrioSig = grafo->obtenerNodo(nodo_final)->getSeccion();
                       if (asignacion_barrios[idBarrioActualCalle] == asignacion_barrios[idBarrioSig]) { // si el barrio es del mismo nodo mpi.

                           pair<int, long> clave = make_pair(v->getId(), idBarrioSig);
                           SegmentoTrayectoVehculoEnBarrio sigSegmento = (*ptr_segmentos_a_recorrer_por_barrio_por_vehiculo)[clave].front();

                           #pragma omp critical(segmentos_a_recorrer_pop_mutex)
                           (*ptr_segmentos_a_recorrer_por_barrio_por_vehiculo)[clave].pop();

                           float peso;
                           auto caminoSigBarrio = grafo->computarCaminoMasCorto(sigSegmento.id_inicio, sigSegmento.id_fin, sigSegmento.id_barrio, peso); //ToDo mejorar que solo busque en el barrio
                           v->setRuta(caminoSigBarrio, sigSegmento.is_segmento_final);
                           v->set_indice_calle_recorrida(0);

                           string idSigCalle = Calle::getIdCalle(caminoSigBarrio[0], caminoSigBarrio[1]);
                           Calle *sigCalle = mapa_barrio[idBarrioSig]->obtenerCalle(idSigCalle);
                           v->setEsperandoTrasladoEntreCalles(true);
                           v->setContadorDePasienciaActivado(false);


                           sigCalle->insertarSolicitudTranspaso(nodo_inicial, nodo_final, v);
                       }else {
                           SolicitudTranspaso solicitudTranspaso;
                           solicitudTranspaso.id_vehiculo = v->getId();
                           solicitudTranspaso.id_barrio = idBarrioSig;
                           solicitudTranspaso.trayectoriaTotal = v->getDistanciaRecorrida();
                           solicitudTranspaso.epocaInicial = v->getEpocaInicio();
                           solicitudTranspaso.id_nodo_inicial_calle_anterior = nodo_inicial;

                           v->setEsperandoTrasladoEntreCalles(true);
                           v->setContadorDePasienciaActivado(false);

                           #pragma omp critical(enviar_solicitud_fn_mutex)
                           this->enviarSolicitudFn(solicitudTranspaso);
                       }
                   }
               }else { // continua computando calles dentro del mismo barrio.
                   v->setEsperandoTrasladoEntreCalles(true);
                   v->setContadorDePasienciaActivado(true);

                    //ToDo hacer que esto sea random
                   long idBarrio = grafo->obtenerNodo(nodo_final)->getSeccion();
                   long idSiguienteNodo = v->sigNodoARecorrer();
                   string codigoSiguienteCalle = Calle::getIdCalle(nodo_final, idSiguienteNodo);
                   Calle *sigCalle = mapa_barrio[idBarrio]->obtenerCalle(codigoSiguienteCalle);

                   v->setNumeroNodoCalleEnEspera(idSiguienteNodo);


                   sigCalle->insertarSolicitudTranspaso(nodo_inicial, nodo_final, v);
               }
           } else {

               if(v->getNumeroIntentosCambioDeRuta() == NUMERO_MAXIMO_INTENTO_CAMBIO_CARRILES){
                   //printf("Desaparesco! %d\n", v->getId());

                   v->setEsperandoTrasladoEntreCalles(false);
                   v->setContadorDePasienciaActivado(false);
                   v->setNumeroEpocasAntesDeCambio(0);
                   v->setNumeroIntentosCambioDeRuta(0);
                   posiciones_vehiculos_en_calle.erase(v->getId());

                   bool vehiculo_insertado_en_otro_nodo_mpi = mi_notificacion.second;

                   if(vehiculo_insertado_en_otro_nodo_mpi ){
                       delete v;
                   }

                   continue;

               }

               if(v->isContadorDePasienciaActivado()){
                   v->setNumeroEpocasAntesDeCambio(v->getNumeroEpocasAntesDeCambio() - 1); //Descuento 1 antes de formzar el cambio

                   #pragma omp critical(debugg) //ToDo quitar eso que no es necesario
                   if(v->getNumeroEpocasAntesDeCambio() == 0){
                       // -- Se buscan caminos alternativos

                       v->setNumeroIntentosCambioDeRuta(v->getNumeroIntentosCambioDeRuta() + 1);

                       Nodo* nodoFinalCalleActual = grafo->obtenerNodo(nodo_final);
                       long ultimoNodoDeLaSubRuta = v->obtenerUltimoNodoDeLaRuta();

                       float minimoPeso = 0;
                       bool isPrimerCaminoExplorado = true;
                       long nodoInicioSigCaminoMinimo;
                       vector<long> rutaMimima;
                       for(auto nv: nodoFinalCalleActual->getNodosVecinos()){

                           Nodo* nodoVecinoCantidado = nv.first;
                           float costoArista = nv.second;

                           if(nodoVecinoCantidado->getIdExt() == nodo_inicial){
                               // No permitidmos que vuelva hacia atras por la calle en la que fue, o sea, no vuelta en U
                               continue;
                           }

                           float pesoPorEseCamino = 0.f;
                           vector<long> ruta;
                           if(nodoVecinoCantidado->getIdExt() == ultimoNodoDeLaSubRuta){ //Caso borde, se tranca en la ultima calle que debe recorrer
                               ruta.push_back(ultimoNodoDeLaSubRuta);
                           } else {
                               ruta = grafo->computarCaminoMasCorto(nodoVecinoCantidado->getIdExt(), ultimoNodoDeLaSubRuta, nodoFinalCalleActual->getSeccion(), pesoPorEseCamino);
                               if(ruta.empty()) continue;
                           }

                           pesoPorEseCamino += costoArista;
                            if(isPrimerCaminoExplorado || pesoPorEseCamino < minimoPeso){
                                isPrimerCaminoExplorado = false;
                                minimoPeso = pesoPorEseCamino;
                                rutaMimima = ruta;
                                nodoInicioSigCaminoMinimo = nodoVecinoCantidado->getIdExt();
                            }
                       }

                       Barrio* barrio = mapa_barrio[nodoFinalCalleActual->getSeccion()];

                       if(nodoInicioSigCaminoMinimo != v->getNumeroNodoCalleEnEspera()){
                           string idCalleDondeSeIntentabaIngresar = Calle::getIdCalle(nodo_final, v->getNumeroNodoCalleEnEspera());
                           Calle* calleDondeSeIntentabaIngresar = mapa_barrio[nodoFinalCalleActual->getSeccion()]->obtenerCalle(idCalleDondeSeIntentabaIngresar);
                           bool solicitudeRetirada = calleDondeSeIntentabaIngresar->consultarSolicitudActivaYRemover(v->getId());
                           if (solicitudeRetirada){ // En caso contrario es que es que la solicitud fue aceptada y es cuestion de que llege la notificacion
                              rutaMimima.insert(rutaMimima.begin(), nodo_final);
                              v->setRuta(rutaMimima, v->get_is_segmento_final());
                               v->set_indice_calle_recorrida(0);

                               string idSigCalle = Calle::getIdCalle(nodo_final, nodoInicioSigCaminoMinimo);
                               Calle* sigCalle = mapa_barrio[nodoFinalCalleActual->getSeccion()]->obtenerCalle(idSigCalle);
                               sigCalle->insertarSolicitudTranspaso(nodo_inicial, nodo_final, v);
                               v->setNumeroNodoCalleEnEspera(nodoInicioSigCaminoMinimo);
                           }
                       }
                       v->setNumeroEpocasAntesDeCambio(NUMERO_EPOCAS_ANTE_DE_REACALCULO_DE_RUTAS);
                   }
               }
           }
        }

        float velocidadActual = ((float)nuevaPosicion - posicion) * (3.6f) / (tiempo_epoca / 1000.f);
        velocidadesVehiculos.push_back(velocidadActual);

        v->setVelocidad(velocidadActual);

        posiciones_vehiculos_en_calle[v->getId()] = nuevaCarilPosicion;

        maximoPorCarril[numeroCarril] = nuevaPosicion - LARGO_VEHICULO;

        vehiculos_ordenados_en_calle_aux.push_back(v);
    }

    vehculos_ordenados_en_calle.clear();
    for(auto v: vehiculos_ordenados_en_calle_aux){
        vehculos_ordenados_en_calle.push_back(v);
    }



    // 3- aceptar vehiculo solicitante, en principio lo hacemos naive aceptando el primero de la cola.

    omp_set_lock(&lock_solicitud);

    if(!solicitudes_traspaso_calle.empty()){

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

                    vehiculoIngresado->setCalleactual(this);

                    pair<int, float> carrilPosicion;
                    carrilPosicion.first = num_carril;
                    carrilPosicion.second = 0.f;

                    posiciones_vehiculos_en_calle[vehiculoIngresado->getId()] = carrilPosicion;

                    vehiculoIngresado->setDistanciaRecorrida(vehiculoIngresado->getDistanciaRecorrida() + largo);

                    vehculos_ordenados_en_calle.push_back(vehiculoIngresado);

                    if(isCalleNula(solicitud.first)){
                        vehiculoIngresado->setEpocaInicio(numeroEpoca);
                    } else {
                        long idNodoInicialNotificante = solicitud.first.first;
                        long idNodoFinalNotificante = solicitud.first.second;
                        long idBarrioCalleANotificar = grafo->obtenerNodo(idNodoInicialNotificante)->getSeccion();
                        long idBarrioActual = grafo->obtenerNodo(nodo_inicial)->getSeccion();

                        if(asignacion_barrios[idBarrioCalleANotificar] != asignacion_barrios[idBarrioActual]){

                            NotificacionTranspaso notificacion;
                            notificacion.id_vehiculo = vehiculoIngresado->getId();
                            notificacion.id_barrio = idBarrioCalleANotificar;

                            #pragma omp critical(enviar_notificacion_fn_mutex)
                            enviarNotificacionFn(notificacion);

                        } else {
                            string idCalleANotificar = Calle::getIdCalle(idNodoInicialNotificante, idNodoFinalNotificante);
                            Calle* calleANotificar = mapa_barrio[idBarrioCalleANotificar]->obtenerCalle(idCalleANotificar);
                            calleANotificar->notificarTranspasoCompleto(solicitud.second->getId(), false);

                        }


                    }
                }
            }
        }





    }

    omp_unset_lock(&lock_solicitud);

    delete[] maximoPorCarril;


    float valorCongestion = calcularCongestion();
    float valorVelocidadMedia = calcularVelocidadMedia(velocidadesVehiculos);
    float valorTiempoMedioDefault = largo / (float)(velocidad_maxima * (float)numero_carriles);

    //float tiempoMedioEsperado = (valorCongestion*0.25f) * (largo / valorVelocidadMedia) + (1-valorCongestion*0.75f) * valorTiempoMedioDefault;
    float tiempoMedioEsperado = valorTiempoMedioDefault + valorTiempoMedioDefault * valorCongestion * ( (velocidad_maxima - valorVelocidadMedia) / velocidad_maxima) * 1.00;


    medicion_costo.push_back(tiempoMedioEsperado);
}

bool Calle::consultarSolicitudActivaYRemover(int idVehiculo){

    omp_set_lock(&lock_solicitud); //Mutex al insertar solicutud

    bool encontrado = false;
    int indice = 0;
    for (auto s: solicitudes_traspaso_calle){
        if(s.second->getId() == idVehiculo){
            encontrado = true;
            break;
        }
        indice++;
    }
    if(encontrado){
        solicitudes_traspaso_calle.erase(solicitudes_traspaso_calle.begin() + indice);
    }
    omp_unset_lock(&lock_solicitud);

    return encontrado;
}


float Calle::obtenerNuevoCostoYLimpiarMedidas(){
    float promedio = 0.f;
    for(auto v: medicion_costo)
        promedio += v;
    promedio = promedio / (float)medicion_costo.size();
    medicion_costo.clear();

    return promedio;
}

void Calle::mostrarEstado(){

    if(vehculos_ordenados_en_calle.empty() && solicitudes_traspaso_calle.empty()){
        return;
    }

    string idCalle = Calle::getIdCalle(this);

    float valorTiempoMedioDefault = largo / (float)(velocidad_maxima * sqrt(numero_carriles));

    Nodo* nodo = grafo->obtenerNodo(nodo_inicial);

    float costoActual;
    for(auto n : nodo->getNodosVecinos()){
        if(n.first->getIdExt() == nodo_final){
            costoActual = n.second;
        }
    }

    LOG(INFO) << " >>>>>>>>>>>  Info calle: " << idCalle << " cantidad_carriles:" << this->numero_carriles << "largo: " << this->largo << " COSTO Original " << valorTiempoMedioDefault << " Costo actual " << costoActual; ;

    for( Vehiculo* v: vehculos_ordenados_en_calle){
        auto carrilPosicion = posiciones_vehiculos_en_calle[v->getId()];

        LOG(INFO) << " id = " << v->getId() << " | carril = " << carrilPosicion.first
        << " | posicion= " << carrilPosicion.second << " tope:" << this->largo << " | velocidad = " << v->getVelocidad() << "Km";
    }

    LOG(INFO) << " ##  SOLICITUDES DE AUTOS CALLE:  " << getIdCalle(this) << " : " <<  this->solicitudes_traspaso_calle.size();

    for( auto a : solicitudes_traspaso_calle){
        LOG(INFO) << " id = " << a.second->getId() << " calle-origen :" << a.first.first << "-" << a.first.second;
    }

    LOG(INFO) << endl;


}

Calle::~Calle(){
    omp_destroy_lock(&lock_solicitud);
    omp_destroy_lock(&lock_notificacion);
}
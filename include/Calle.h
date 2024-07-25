
#include "Utils.h"
#include "easylogging++.h"
#include "queue"
#include "map"
#include "vector"
#include "string"
#include "set"
#include "omp.h"
#include "Grafo.h"

#include "Constantes.h"

#ifndef CALLE_H
#define CALLE_H

using namespace std;

class Vehiculo;
class Barrio;
class Calle {

private:
    long nodo_inicial, nodo_final;
    float largo;
    unsigned int numero_carriles;
    float velocidad_maxima;
    Grafo g;

    int cantidad_epocas_parado = 0;
    int * id_ultimos_vehiculos_tope;
    int carril_a_actualizar = 0;
    bool trancado = true;

    vector<pair<pair<long,long>, Vehiculo*>> solicitudes_traspaso_calle;
    set<pair<int, bool>>notificaciones_traslado_calle_realizado;

    //Dado un id_vehiculo se guarda el numero de carril y su pociion dentro del caril, donde la posicion es el frente del vehiculo dado que 0 es el inicio de la calle
    map<unsigned int,  pair<int , float>> posiciones_vehiculos_en_calle;

    // Lista de vehiculos dentro de la calle, ordenadados por posicion (de mayor a menor), se actualizan en este orden.
    vector<Vehiculo*> vehculos_ordenados_en_calle;

    function<void(float, int)> doneFn;
    function<void(SolicitudTranspaso&)> enviarSolicitudFn;
    function<void(NotificacionTranspaso &)> enviarNotificacionFn;

    Grafo* grafo;
    map<long, Barrio*> mapa_barrio;
    map<long, int> asignacion_barrios;

    map<pair<int, long>, queue<SegmentoTrayectoVehculoEnBarrio>> * ptr_segmentos_a_recorrer_por_barrio_por_vehiculo;

    int my_rank;

    omp_lock_t lock_solicitud;
    omp_lock_t lock_notificacion;

public:
    Calle(long id_nodo_inicial, long id_nodo_final, float largo, unsigned numero_carriles,float velocidad_maxima,
          map<long, Barrio*> & mapa_barrio,
          function<void(float, int)>& doneFn,
          function<void(SolicitudTranspaso&)>& enviarSolicitudFn,
          function<void(NotificacionTranspaso &)>& enviarNotificacionFn,
          Grafo* grafo,
          map<long, int>& asignacion_barrios,
          map<pair<int, long>, queue<SegmentoTrayectoVehculoEnBarrio>> * ptr_segmentos_a_recorrer_por_barrio_por_vehiculo,
          int my_rank);

    void insertarSolicitudTranspaso(long id_inicio_calle_solicitante, long id_fin_calle_solicitante, Vehiculo* vehiculo);

    void notificarTranspasoCompleto(int idVehiculo, bool eliminar_luego_de_notificar);

    // tiempo_epoca en ms
    void ejecutarEpoca(float tiempo_epoca, int numeroEpoca);

    void mostrarEstado();

    static string getIdCalle(Calle* calle){
        return to_string(calle->nodo_inicial) + "-" + to_string(calle->nodo_final);
    }

    static string getIdCalle(long nodo_inicial, long nodo_final){
        return to_string(nodo_inicial)+ "-" + to_string(nodo_final);
    }

    ~Calle();


};

#include "Vehiculo.h"
#include "Barrio.h"

#endif //CALLE_H


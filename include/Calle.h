
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

    vector<pair<pair<long,long>, Vehiculo*>> solicitudes_traspaso_calle;
    set<pair<int, bool>>notificaciones_traslado_calle_realizado;

    //Dado un id_vehiculo se guarda el numero de carril y su pociion dentro del caril, donde la posicion es el frente del vehiculo dado que 0 es el inicio de la calle
    map<unsigned int,  pair<int , float>> posiciones_vehiculos_en_calle;

    // Lista de vehiculos dentro de la calle, ordenadados por posicion (de mayor a menor), se actualizan en este orden.
    vector<Vehiculo*> vehculos_ordenados_en_calle;

    function<void(float, int,long,long)> doneFn;
    function<void(SolicitudTranspaso&)> enviarSolicitudFn;
    function<void(NotificacionTranspaso &)> enviarNotificacionFn;

    map<int, Calle*> vehiculos_con_solicitud_enviada;

    //Conjunto con todos los vehiculos cuya solicitud se verifico aceptada al no encontrarse mas en la lista de solicitudes
    // de la calle a la que se solicito

    set<int> vehiculos_con_solicitud_aceptada_esperando_notificacion;
    map<int, int> numero_intentos_restantes_re_entruamiento;
    map<int, int> numero_epocas_restantes_antes_de_proximo_intento_de_reenrutamiento;


    Grafo* grafo;
    map<long, Barrio*> mapa_barrio;
    map<long, int> asignacion_barrios;

    map<pair<int, long>, queue<SegmentoTrayectoVehculoEnBarrio>> * ptr_segmentos_a_recorrer_por_barrio_por_vehiculo;

    int my_rank;

    vector<float> medicion_costo;

    omp_lock_t lock_solicitud;
    omp_lock_t lock_notificacion;


    void remover_vehiculo_calle(Vehiculo* v, bool eliminarInstancia);

    float calcularCongestion();
    float calcularVelocidadMedia(vector<float> & velocidades);


public:
    Calle(long id_nodo_inicial, long id_nodo_final, float largo, unsigned numero_carriles,float velocidad_maxima,
          map<long, Barrio*> & mapa_barrio,
          function<void(float, int,long,long)>& doneFn,
          function<void(SolicitudTranspaso&)>& enviarSolicitudFn,
          function<void(NotificacionTranspaso &)>& enviarNotificacionFn,
          Grafo* grafo,
          map<long, int>& asignacion_barrios,
          map<pair<int, long>, queue<SegmentoTrayectoVehculoEnBarrio>> * ptr_segmentos_a_recorrer_por_barrio_por_vehiculo,
          int my_rank);

    void insertarSolicitudTranspaso(long id_inicio_calle_solicitante, long id_fin_calle_solicitante, Vehiculo* vehiculo);

    //Retorna true si la solicitud seguia en la lista de solicitudes, y en ese caso es eliminada de la lista
    bool consultarSolicitudActivaYRemover(int idVehiculo);

    bool notificarTranspasoCompleto(int idVehiculo, bool eliminar_luego_de_notificar);

    // tiempo_epoca en ms
    void ejecutarEpoca(float tiempo_epoca, int numeroEpoca);

    void mostrarEstado();

    float obtenerNuevoCostoYLimpiarMedidas();

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


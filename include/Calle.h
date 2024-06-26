#include "queue"
#include "Vehiculo.h"
#include "map"
#include "vector"
#include "string"
#include "set"
#include "Constantes.h"


#ifndef CALLE_H
#define CALLE_H


using namespace std;
class Calle {

private:
    unsigned int nodo_inicial, nodo_final;
    float largo;
    unsigned int numero_carriles;
    float velocidad_maxima;

    queue<pair<Calle*, Vehiculo*>> solicitudes_traspaso_calle;
    set<unsigned int>notificaciones_traslado_calle_realizado;

    //Dado un id_vehiculo se guarda el numero de carril y su pociion dentro del caril, donde la posicion es el frente del vehiculo dado que 0 es el inicio de la calle
    map<unsigned int,  pair<int , float>> posiciones_vehiculos_en_calle;

    // Lista de vehiculos dentro de la calle, ordenadados por posicion (de mayor a menor), se actualizan en este orden.
    vector<Vehiculo*> vehculos_ordenados_en_calle;


public:
    Calle(unsigned int id_nodo_inicial, unsigned int id_nodo_final, float largo, unsigned numero_carriles, float velocidad_maxima);

    void insertarSolicitudTranspaso(Calle* calleSolicitante, Vehiculo* vehiculo);

    void notificarTranspasoCompleto(unsigned int idVehiculo);

    // tiempo_epoca en ms
    void ejecutarEpoca(float tiempo_epoca);

    void mostrarEstado();

    static string getIdCalle(Calle* calle){
        return to_string(calle->nodo_inicial) + "-" + to_string(calle->nodo_final);
    }




};



#endif //CALLE_H

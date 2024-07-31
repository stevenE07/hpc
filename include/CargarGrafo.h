#include "Grafo.h"
#include "map"
#include "Calle.h"
#include "Barrio.h"

#ifndef CARGARGRAFO_H
#define CARGARGRAFO_H

#include <fstream>

#include <json.hpp>

using json = nlohmann::json;
using namespace std;

class CargarGrafo {

private:
    json data;
public:
    CargarGrafo(string file);

    vector<pair<long, string>> obtenerBarrios();

    void FormarGrafo(Grafo* grafo,
                     map<long, Barrio*>& barrios,
                     std::function<void(float, int,long)>& doneFn,
                     function<void(SolicitudTranspaso&)>& enviarSolicitudFn,
                     function<void(NotificacionTranspaso &)>& enviarNotificacionFn,
                     map<long, int>&  asignacion_barrios,
                     map<pair<int, long>, queue<SegmentoTrayectoVehculoEnBarrio>>* ptr_segmentos_a_recorrer_por_barrio_por_vehiculo,
                     int my_rank );
};



#endif //CARGARGRAFO_H

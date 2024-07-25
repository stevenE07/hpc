
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <cmath>
#include "random"
#include "vector"

#ifndef HPC_UTILS_H
#define HPC_UTILS_H

# define M_PI 3.14159265358979323846

// ----------- Struct con datos a enviar
typedef struct {
    int id_vehiculo;
    long id_inicio;
    long id_fin;
    long id_barrio;
    char is_segmento_final;
}SegmentoTrayectoVehculoEnBarrio;

typedef struct {
    int id_vehiculo;
    int epocaInicial;
    float trayectoriaTotal;
    long id_nodo_inicial_calle_anterior;
    long id_barrio;
}SolicitudTranspaso;

typedef struct {
    int id_vehiculo;
    long id_barrio;
}NotificacionTranspaso;

 typedef struct{
    long id_barrio;
    int cantidad_vehiculos_a_generar;
    int nodo_responsable;
}Asignacion_barrio;




inline double getDistanceEntrePuntosEnMetros(double latitude1, double longitude1, double latitude2, double longitude2) {
    double theta = longitude1 - longitude2;
    double distance = 60 * 1.1515 * (180/M_PI) * acos(
            sin(latitude1 * (M_PI/180)) * sin(latitude2 * (M_PI/180)) +
            cos(latitude1 * (M_PI/180)) * cos(latitude2 * (M_PI/180)) * cos(theta * (M_PI/180))
    );

    return round(distance * 1.609344 * 1000);

}

inline int getClasePorProbailidad(std::mt19937& rnd, std::vector<double>& pro){
    std::uniform_real_distribution<float> dist(0, 1);
    float probailidad = dist(rnd);

    float acc = 0.0f;
    int c = 0;
    while( c <pro.size() && ! (probailidad >= acc && probailidad < acc + pro[c] ) ){
        acc += pro[c];
        c ++;
    }

    if( c == pro.size()){
        return -1;
    } else {
        return c;
    }
}






#endif //HPC_UTILS_H

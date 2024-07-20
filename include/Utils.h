

#ifndef HPC_UTILS_H
#define HPC_UTILS_H


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
    long id_nodo_inicial_calle_anterior;
    long id_barrio;
}SolicitudTranspaso;

typedef struct {
    int id_vehiculo;
}NotificacionTranspaso;



#endif //HPC_UTILS_H

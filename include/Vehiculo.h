#include "easylogging++.h"
#include "vector"
#include "string"
#include "omp.h"

#ifndef VEHICULO_H
#define VEHICULO_H

using namespace std;

class Calle;
class Vehiculo {
private:
    //Propiedades del vehiculo
    int id;

    long id_barrio_inicio;

    //Dirrecion
    std::vector<long> ruta;

    unsigned int indice_calle_recorrida; //Es 0 cuando esta en su primera calle
    unsigned char is_segmento_final;

    Calle* calleactual;

    //Aspectos temporales
    float velocidad;

    bool esperando_notificacion;

    float distancia_recorrida;
    int epoca_inicio;


    omp_lock_t lock_esperando_notificacion;

public:




    Vehiculo(int id, long barrio_inicio);

    ~Vehiculo();

    long getIdBarrioInicio() const;

    void setIdBarrioInicio(long idBarrioInicio);


    Calle *getCalleactual() const;

    void setCalleactual(Calle *calleactual);

    int getId() const;

    void setId(unsigned int id);

    float getVelocidad() const;

    void setVelocidad(float velocidad);

    unsigned int getNumeroCalleRecorrida() const;
    void  set_indice_calle_recorrida(int valor);
    void setNumeroCalleRecorrida(unsigned int numeroCalleRecorrida);

    long sigNodoARecorrer();

    const vector<long> &getRuta() const;

    unsigned char get_is_segmento_final();

    void setRuta(const vector<long> &ruta, unsigned char  is_segmento_fina);

    long nodo_destino();

    void imprimirRuta();

    long nodo_actual();


    float getDistanciaRecorrida() const;

    void setDistanciaRecorrida(float distanciaRecorrida);

    int getEpocaInicio() const;

    void setEpocaInicio(int epocaInicio);

    bool isEsperandoNotificacion() const;

    void setEsperandoNotificacion(bool esperandoNotificacion);


    long obtenerUltimoNodoDeLaRuta(){
        return ruta[ruta.size() - 1];
    }

};


#include "Calle.h"

#endif //VEHICULO_H

#include "easylogging++.h"
#include "vector"
#include "string"

#ifndef VEHICULO_H
#define VEHICULO_H

using namespace std;

class Calle;
class Vehiculo {
private:
    //Propiedades del vehiculo
    int id;

    //Dirrecion
    std::vector<long> ruta;

    unsigned int indice_calle_recorrida; //Es 0 cuando esta en su primera calle
    unsigned char is_segmento_final;

    Calle* calleactual;

    //Aspectos temporales
    float velocidad;

    bool esperando_traslado_entre_calles;

    bool contadorDePasienciaActivado;
    int numeroEpocasAntesDeCambio;
    long numeroNodoCalleEnEspera;
    int numeroIntentosCambioDeRuta;

    float distancia_recorrida;
    int epoca_inicio;


public:

    long id_barrio_inicio = 0;
    long id_barrio_final = 0;

    Vehiculo(int id);

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

    bool isEsperandoTrasladoEntreCalles() const;

    void setEsperandoTrasladoEntreCalles(bool esperandoTrasladoEntreCalles);

    const vector<long> &getRuta() const;

    unsigned char get_is_segmento_final();

    void setRuta(const vector<long> &ruta, unsigned char  is_segmento_fina);

    long nodo_destino();

    void imprimirRuta();

    long nodo_actual();

    long getNumeroNodoCalleEnEspera() const;

    void setNumeroNodoCalleEnEspera(long numeroNodoCalleEnEspera);

    bool isContadorDePasienciaActivado() const;

    void setContadorDePasienciaActivado(bool contadorDePasienciaActivado);

    int getNumeroEpocasAntesDeCambio() const;

    void setNumeroEpocasAntesDeCambio(int numeroEpocasAntesDeCambio);

    float getDistanciaRecorrida() const;

    void setDistanciaRecorrida(float distanciaRecorrida);

    int getEpocaInicio() const;

    void setEpocaInicio(int epocaInicio);

    int getNumeroIntentosCambioDeRuta() const;

    void setNumeroIntentosCambioDeRuta(int numeroIntentosCambioDeRuta);

    long obtenerUltimoNodoDeLaRuta(){
        return ruta[ruta.size() - 1];
    }

};


#include "Calle.h"

#endif //VEHICULO_H

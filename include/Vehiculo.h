#include "easylogging++.h"
#include "vector"
#include "string"

#ifndef VEHICULO_H
#define VEHICULO_H

using namespace std;

class Vehiculo {
private:
    //Propiedades del vehiculo
    int id;

    long epoca_inicial;
    long epoca_final;

    //Dirrecion
    std::vector<long> ruta;

    unsigned int indice_calle_recorrida; //Es 0 cuando esta en su primera calle
    unsigned char is_segmento_final;

    unsigned char sss;

    //Aspectos temporales
    float velocidad;

    bool esperando_traslado_entre_calles;

public:



    Vehiculo(int id, long epoca_inicial,float velocidad);

    unsigned int getId() const;

    void setId(unsigned int id);

    unsigned long getEpocaInicial() const;

    void setEpocaInicial(unsigned long epocaInicial);

    unsigned long getEpocaFinal() const;

    void setEpocaFinal(unsigned long epocaFinal);

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

    unsigned char getIsSegmentoFinal() const;

    void setIsSegmentoFinal(unsigned char isSegmentoFinal);
};



#endif //VEHICULO_H

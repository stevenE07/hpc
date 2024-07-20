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

    void setNumeroCalleRecorrida(unsigned int numeroCalleRecorrida);

    long sigNodoARecorrer();

    bool isEsperandoTrasladoEntreCalles() const;

    void setEsperandoTrasladoEntreCalles(bool esperandoTrasladoEntreCalles);

    const vector<long> &getRuta() const;

    void setRuta(const vector<long> &ruta);

    long nodo_destino();

    void imprimirRuta();

    long nodo_actual();
};



#endif //VEHICULO_H

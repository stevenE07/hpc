#include "vector"
#include "string"

#ifndef VEHICULO_H
#define VEHICULO_H

using namespace std;

class Vehiculo {
private:
    //Propiedades del vehiculo
    unsigned int id;

    unsigned long epoca_inicial;
    unsigned long epoca_final;

    //Dirrecion
    std::vector<unsigned int> ruta;
    unsigned int indice_calle_recorrida; //Es 0 cuando esta en su primera calle

    //Aspectos temporales
    float velocidad;

    bool esperando_traslado_entre_calles;

public:

    Vehiculo(int id,unsigned long epoca_inicial,float velocidad );

    unsigned int getId() const;

    void setId(unsigned int id);

    unsigned long getEpocaInicial() const;

    void setEpocaInicial(unsigned long epocaInicial);

    unsigned long getEpocaFinal() const;

    void setEpocaFinal(unsigned long epocaFinal);

    const std::vector<unsigned int> &getRuta() const;

    void setRuta(const std::vector<unsigned int> &ruta);

    float getVelocidad() const;

    void setVelocidad(float velocidad);

    unsigned int getNumeroCalleRecorrida() const;

    void setNumeroCalleRecorrida(unsigned int numeroCalleRecorrida);

    std::string sigCalleARecorrer();

    bool isEsperandoTrasladoEntreCalles() const;

    void setEsperandoTrasladoEntreCalles(bool esperandoTrasladoEntreCalles);


};



#endif //VEHICULO_H

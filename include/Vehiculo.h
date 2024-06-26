#include "vector"

#ifndef VEHICULO_H
#define VEHICULO_H

class Vehiculo {
private:
    //Propiedades del vehiculo
    unsigned int id;

    unsigned long epoca_inicial;
    unsigned long epoca_final;

    //Dirrecion
    std::vector<unsigned int> ruta;

    //Aspectos temporales
    float velocidad;

public:

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

};



#endif //VEHICULO_H

//
// Created by Steven on 27/06/2024.
//

#include "Grafo.h"
#include "map"
#include "Calle.h"

#ifndef CARGARGRAFO_H
#define CARGARGRAFO_H

#include <fstream>

#include <json.hpp>

using json = nlohmann::json;

class CargarGrafo {

private:
    json data;
public:
    CargarGrafo(std::string file);
    void leerDatos(Grafo* grafo, std::map<std::string, Calle*>& calles, function<Calle*(string)> & obtenerCallePorIdFn, std::function<void()>& doneFn );
};



#endif //CARGARGRAFO_H

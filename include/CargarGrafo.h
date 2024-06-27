//
// Created by Steven on 27/06/2024.
//

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
};



#endif //CARGARGRAFO_H

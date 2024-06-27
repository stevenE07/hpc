//
// Created by Steven on 27/06/2024.
//

#include "../include/CargarGrafo.h"

#include <iostream>

CargarGrafo::CargarGrafo(std::string file) {
    std::ifstream f(file);
    data = json::parse(f);
    //std::cout << data << std::endl;

    for (size_t i = 0; i < data["nodes"].size(); ++i) {
        std::cout << "Node " << i << ": " << data["nodes"][i]["x"] << std::endl;
    }


}

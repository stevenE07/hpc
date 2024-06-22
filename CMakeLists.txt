cmake_minimum_required(VERSION 3.27)
project(hpc)

set(CMAKE_CXX_STANDARD 17)

find_package(OpenMP REQUIRED)
#find_package(Boost REQUIRED COMPONENTS graph)

add_executable(hpc src/main.cpp
        src/Calle.cpp
        include/Calle.h
        src/Vehiculo.cpp
        include/Vehiculo.h
        src/Grafo.cpp
        include/Grafo.h
        include/Constantes.h)

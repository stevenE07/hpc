cmake_minimum_required(VERSION 3.27)
project(hpc)

set(PROJECT_BASE_DIR ${CMAKE_SOURCE_DIR})

set(CMAKE_CXX_STANDARD 17)


#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")

# Encontrar OpenMP
find_package(OpenMP REQUIRED)
if (OpenMP_CXX_FOUND)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()

find_package(MPI REQUIRED)
if (MPI_CXX_FOUND)
    include_directories(${MPI_INCLUDE_PATH})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MPI_CXX_COMPILE_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${MPI_CXX_LINK_FLAGS}")
endif()

# Directorio donde se encuentra Easylogging++
set(EASYLOGGINGPP_DIR ${CMAKE_SOURCE_DIR}/external/easyloggingpp/src)
include_directories(${EASYLOGGINGPP_DIR})

set(NLOHMANN_JSON_DIR ${CMAKE_SOURCE_DIR}/external/nlohmann)
include_directories(${NLOHMANN_JSON_DIR})

add_definitions(-DPROJECT_BASE_DIR="${PROJECT_BASE_DIR}")


# Agregar ejecutable
add_executable(hpc
        main.cpp
        src/Barrio.cpp
        src/Calle.cpp
        src/Vehiculo.cpp
        src/Grafo.cpp
        include/Constantes.h
        src/CargarGrafo.cpp
        include/CargarGrafo.h
        include/Utils.h
)

# Enlazar bibliotecas necesarias (como OpenMP, Boost, etc.)
target_link_libraries(hpc
        OpenMP::OpenMP_CXX
        ${EASYLOGGINGPP_DIR}/easylogging++.cc
        MPI::MPI_CXX
)

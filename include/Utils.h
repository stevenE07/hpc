
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <cmath>
#include "random"
#include "vector"
#include <algorithm>
#ifndef HPC_UTILS_H
#define HPC_UTILS_H

#include <json.hpp>

using json = nlohmann::json;
using namespace std;

# define M_PI 3.14159265358979323846

using namespace std;

// ----------- Struct con datos a enviar
typedef struct {
    int id_vehiculo;
    long id_inicio;
    long id_fin;
    long id_barrio;
    char is_segmento_final;
}SegmentoTrayectoVehculoEnBarrio;

typedef struct {
    int id_vehiculo;
    int epocaInicial;
    float trayectoriaTotal;
    long id_nodo_inicial_calle_anterior;
    long id_barrio;
    long barrio_inicio;
}SolicitudTranspaso;

typedef struct {
    int id_vehiculo;
    long id_barrio;
}NotificacionTranspaso;

 typedef struct{
    long id_barrio;
    int cantidad_vehiculos_a_generar;
    int nodo_responsable;
}Asignacion_barrio;




inline float getDistanceEntrePuntosEnMetros(double latitude1, double longitude1, double latitude2, double longitude2) {
    double theta = longitude1 - longitude2;
    double distance = 60 * 1.1515 * (180/M_PI) * acos(
            sin(latitude1 * (M_PI/180)) * sin(latitude2 * (M_PI/180)) +
            cos(latitude1 * (M_PI/180)) * cos(latitude2 * (M_PI/180)) * cos(theta * (M_PI/180))
    );

    return (float)round(distance * 1.609344 * 1000);

}

inline int getClasePorProbailidad(std::mt19937& rnd, std::vector<double>& pro){
    std::uniform_real_distribution<float> dist(0, 1);
    float probailidad = dist(rnd);

    float acc = 0.0f;
    int c = 0;
    while( c <pro.size() && ! (probailidad >= acc && probailidad < acc + pro[c] ) ){
        acc += pro[c];
        c ++;
    }

    if( c == pro.size()){
        return -1;
    } else {
        return c;
    }
}

inline void calculo_equitativo(int size_mpi, int my_rank, std::map<long,int> & cantidad_vehiculos_a_generar_por_barrio, std::map<long,int> & asignacion_barrios, vector<long> & mis_barrios) {
    vector<pair<long,int>> cantidades_por_barrio_ordenadas;
    for(auto info : cantidad_vehiculos_a_generar_por_barrio) {
        cantidades_por_barrio_ordenadas.push_back({info.first,info.second});
    }

    std::sort(cantidades_por_barrio_ordenadas.begin(), cantidades_por_barrio_ordenadas.end(),
            [](const std::pair<long, int>& a, const std::pair<long, int>& b) {
                return a.second > b.second;
            });



    // --- Asignacion de barios a nodos
    vector<long> carga_por_nodo(size_mpi,0);
    for(auto cantidad_barrio : cantidades_por_barrio_ordenadas ) {
        long index = min_element(carga_por_nodo.begin(), carga_por_nodo.end()) - carga_por_nodo.begin();
        asignacion_barrios[cantidad_barrio.first ] = index;
        carga_por_nodo[index] += cantidad_barrio.second;

        if(index == my_rank) {
            mis_barrios.push_back(cantidad_barrio.first);
        }
    }
    int index = 0;
    cout << "---- carga por nodo mpi ----" << endl;
    for(auto c : carga_por_nodo) {
        cout << "index:" << index << " cantidad:" << c << endl;
        index++;
    }
}

inline std::map<std::string, std::string> readConfig(const std::string& filename) {
    std::ifstream file(filename);
    std::map<std::string, std::string> config;
    std::string line;

    while (std::getline(file, line)) {
        size_t delimiterPos = line.find('=');
        std::string key = line.substr(0, delimiterPos);
        std::string value = line.substr(delimiterPos + 1);
        config[key] = value;
    }

    return config;
}


inline void calculo_naive_por_nodo_mpi(int my_rank, int size_mpi, vector<pair<long, basic_string<char>>> barrios, std::map<long,int> & asignacion_barrios, vector<long> & mis_barrios) {
    for(int i = 0; i < barrios.size(); i++){  //ToDo Cambiar
        asignacion_barrios[barrios[i].first] = i % size_mpi;
        if(my_rank == i % size_mpi){
            mis_barrios.push_back(barrios[i].first);
        }
    }
}

inline std::vector<std::vector<double>> cargar_matriz_barrios_barrios(string file, int cant_barrios) {
    json data;
    std::ifstream f(file);
    data = json::parse(f);
    std::vector<std::vector<double>> res(cant_barrios , std::vector<double>(cant_barrios , 0));
    for (int i = 0; i < data["probabilidades"].size(); ++i) {
        int x = data["probabilidades"][i]["bi"];
        int y = data["probabilidades"][i]["bf"];
        double prob = data["probabilidades"][i]["prob"];
        res[x][y] = prob;
    }

    for(int i = 0; i < cant_barrios; i++) {
        double valor = 1;
        int cantidad_distintos_cero = 0;
        for(int j = 0; j < cant_barrios; j++) {
            valor =  valor - res[i][j];
            cantidad_distintos_cero ++;
        }
        double prob_faltantes = valor/cantidad_distintos_cero;
        for(int j = 0; j < cant_barrios; j++) {
            if(res[i][j] == 0) {
                res[i][j] = prob_faltantes;
            }
        }
    }

    return res;

}




#endif //HPC_UTILS_H

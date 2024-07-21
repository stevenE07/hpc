
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>

#ifndef HPC_UTILS_H
#define HPC_UTILS_H


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
    long id_nodo_inicial_calle_anterior;
    long id_barrio;
}SolicitudTranspaso;

typedef struct {
    int id_vehiculo;
}NotificacionTranspaso;

struct barrio_cantidad {
    int id_barrio;
    int cantidad;
};

inline void leerCSVbarrioCantidades(const std::string& nombreArchivo, std::map<int, int>& barrios_con_cantidades) {
    std::ifstream file(nombreArchivo);
    if (!file.is_open()) {
        std::cerr << "No se pudo abrir el archivo: " << nombreArchivo << std::endl;
        return;
    }

    std::string line;

    // Omitir la primera línea de encabezado
    std::getline(file, line);

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string id_barrio_str, cantidad_str;

        std::getline(ss, id_barrio_str, ',');
        std::getline(ss, cantidad_str, ',');

        try {
            // Comprobar si las cadenas no están vacías antes de convertirlas
            if (!id_barrio_str.empty() && !cantidad_str.empty()) {
                int id_barrio = std::stoi(id_barrio_str);
                int cantidad = std::stoi(cantidad_str);

                barrios_con_cantidades[id_barrio] = cantidad;
            }
        } catch (const std::invalid_argument& e) {
            std::cerr << "Error de conversión: " << e.what() << " en línea: " << line << std::endl;
        } catch (const std::out_of_range& e) {
            std::cerr << "Número fuera de rango: " << e.what() << " en línea: " << line << std::endl;
        }
    }

    file.close();
}

inline void calcularProbabilidad(const std::map<int, int>& barrios_con_cantidades, std::map<int, double>& probabilidad_por_barrio) {
    int total = 0;
    for (const auto& barrio : barrios_con_cantidades) {
        total += barrio.second;
    }
    for (const auto& barrio : barrios_con_cantidades) {
        probabilidad_por_barrio[barrio.first] = static_cast<double>(barrio.second) / total;
    }
}

inline void asignarCantidades(int total_cantidad, const std::map<int, double>& probabilidad_por_barrio, std::map<int, int>& asignaciones) {
    for (const auto& barrio : probabilidad_por_barrio) {
        asignaciones[barrio.first] = static_cast<int>(barrio.second * total_cantidad);
    }
}






#endif //HPC_UTILS_H

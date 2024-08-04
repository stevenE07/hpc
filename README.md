Para levantar el proyecto es necesario pararse en la carpeta build
y ejecutar el comando:

cmake ..
make
mpirun --np {numero_procesos} --hostfile ../mis_hosts ./hpc


---- Datos del archivo de configuracion /configuraciones/config.txt

omp_set_num_thread : elegir la cantidad de procesos omp
numero_vehiculos_en_curso_global : cantidad de vehiculos en simulacion
modo_balance_carga:
    1 -> balance por vehiculos
    2 -> balance random
    3 -> balance por calles
    4 -> balance por barrios

ciudad:
    1 -> montevideo
    2 -> buenos aires

mostrar_tiempos_promedio_viaje_por_bario_por_barrio: 0 o 1
    si es 1 activa la salida de la matriz barrio por barrio que captura
    las cantitades de promedios de viaje para cada barrio origen destino.
    disclaimer: este valor es válido únicamente para montevideo,
    delo contrario hay que colocar un 0.


Aclaración:
En la carpeta "datos" se encuentran los mapas zipeados, para ser utilizados es necesario descromprimirlos.



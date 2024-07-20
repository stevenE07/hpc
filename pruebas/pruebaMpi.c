#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"
#define N 17000

int main(int argc, char** argv) {
    int pid; // Identificador del proceso
    int a[N], d[N];
    MPI_Status status;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &pid);

    // Calcular el tamaño del buffer necesario
    int bufsize = N * sizeof(int) + MPI_BSEND_OVERHEAD;
    char *buf = (char*)malloc(bufsize);
    MPI_Buffer_attach(buf, bufsize);

    if (pid == 0) {
        // Inicializar el array a con algunos valores
        for (int i = 0; i < N; i++) {
            a[i] = i;
        }
        MPI_Bsend(a, N, MPI_INT, 1, 0, MPI_COMM_WORLD);
        printf("Proceso %d: Envié el mensaje.\n", pid);
    } else {
        MPI_Recv(d, N, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
        printf("Proceso %d: Recibí el mensaje.\n", pid);
    }

    MPI_Buffer_detach(&buf, &bufsize);
    free(buf);
    MPI_Finalize();
    return 0;
}
#include <stdio.h>
#include <mpi.h>

int main (int argc, char *argv[]) {

    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);
    MPI_Comm_size(MPI_COMM_WORLD,&size);

    char* name;
    int* numeroLetras;
    int MPI_Get_processor_name(name, numeroLetras);

    printf("Procesador %s", name);


    printf("Hello world from process %d of %d\n", rank, size);
    MPI_Finalize();
    return 0;
}
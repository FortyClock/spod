#include <stdio.h>
#include <mpi.h>

int main(int argc, char **argv)
{
    int rank;
    int process_count;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &process_count);

    printf(
        "hello by procces %d. Total: %d\n",
        rank,
        process_count
    );

    MPI_Finalize();

    return 0;
}
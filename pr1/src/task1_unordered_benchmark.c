// Автор: Марк Пархоменко
// Дата: 28.10.2026

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

int main(int argc, char **argv)
{
    int rank;
    int process_count;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &process_count);

    long n;
    if (argc != 2)
    {
        if (rank == 0)
            printf("No size of buffer n. Please, enter the n\n");
        MPI_Finalize();
        return 0;
    }

    char *endptr;
    n = strtol(argv[1], &endptr, 10);

    if(*endptr != '\0' || endptr == argv[1] || n <= 0)
    {   
        if (rank == 0)
            printf("Wrong parameter n\n");
        MPI_Finalize();
        return 0;
    }
    
    if (rank == 0)
    {
        printf("Rank: %d\nMessage size: %ld\n", rank, n);

        for (int i = 1; i < process_count; i++)
        {

            int received_rank;
            MPI_Status status;

            MPI_Recv(
                &received_rank,
                1,
                MPI_INT,
                MPI_ANY_SOURCE,
                0,
                MPI_COMM_WORLD,
                &status
            );

            printf("Received rank %d, Sender %d\n",
                received_rank,
                status.MPI_SOURCE);
        }
    }

    else
    {
        MPI_Send(
            &rank,
            1,
            MPI_INT,
            0,
            0,
            MPI_COMM_WORLD
        );
    }

    MPI_Finalize();

    return 0;
}

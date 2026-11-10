// Автор: Марк Пархоменко
// Дата: 27.10.2026

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

    if(argc != 2 )
    {
        if (rank == 0)
            printf("Wrong number of argc\n");
        MPI_Finalize();
        return 1;
    }

    int n = atoi(argv[1]);

    if (n <= 0)
    {
        if(rank == 0)
        {
            printf("Buffer size must be positive\n");
        }
        MPI_Finalize();
        return 1;
    }

    int *buffer = malloc((size_t)n * sizeof(*buffer));
    if (!buffer)
    {
        printf("Failed to allocate buffer\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }

    if (rank != 0)
    {
        for (int i = 0; i < n; i++)
        {
            buffer[i] = rank;
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);
    double  start_time = MPI_Wtime();
    if (rank != 0)
    {

        MPI_Send(
            buffer,
            n,
            MPI_INT,
            0,
            0,
            MPI_COMM_WORLD
        );
    }
    else
    {
        for (int i = 1; i < process_count; i++)
        {
            MPI_Status status;
            MPI_Recv(
                buffer,
                n,
                MPI_INT,
                i,
                0,
                MPI_COMM_WORLD,
                &status
            );
        }
    }

    double elapsed = MPI_Wtime() - start_time;
    double max_elapsed = 0.0;

    MPI_Reduce(
        &elapsed,
        &max_elapsed,
        1,
        MPI_DOUBLE,
        MPI_MAX,
        0,
        MPI_COMM_WORLD
    );

    if (rank == 0)
    {
        printf("processes=%d size=%d time=%.9f\n",
            process_count,
            n,
            max_elapsed);
    }

    free(buffer);
    MPI_Finalize();

    return 0;
}

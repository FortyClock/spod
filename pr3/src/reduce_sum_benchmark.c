// Автор: Марк Пархоменко
// Дата: 10.11.2026

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <mpi.h>

#define MAGIC_NUM 5
#define COUNT_ITER 1000

int main(int argc, char *argv[])
{
    int rank;
    int process_count;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &process_count);

    const size_t count_nums = process_count + MAGIC_NUM;
    int nums_send[count_nums];

    for (size_t i = 0; i < count_nums; i++)
    {
        nums_send[i] = i * rank;
    }
    int nums_recv[count_nums];

    MPI_Barrier(MPI_COMM_WORLD);

    // Начало замеров
    double start_time = MPI_Wtime();
    
    for (int iter = 0; iter < COUNT_ITER; iter++) // COUNT_ITER итераций вызова MPI_Reduce
    {
        MPI_Reduce(
            &nums_send,
            &nums_recv,
            count_nums,
            MPI_INT,
            MPI_SUM,
            0,
            MPI_COMM_WORLD
        );
    }

    double elapsed_time = MPI_Wtime() - start_time;

    // Конец замеров

    double max_elapsed;
    MPI_Reduce(
        &elapsed_time,
        &max_elapsed,
        1,
        MPI_DOUBLE,
        MPI_MAX,
        0,
        MPI_COMM_WORLD
    );

    if (rank == 0)
    {   
        double avg_max_time = max_elapsed / COUNT_ITER;
        printf("Avg max time %.12f seconds\n",
            avg_max_time);
    }

    MPI_Finalize();
    return 0;
}

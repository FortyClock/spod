// Автор: Марк Пархоменко
// Дата: 10.11.2026

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <mpi.h>

#define MAGIC_NUM 5

int main(int argc, char *argv[])
{
    int rank;
    int process_count;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &process_count);

    const size_t count_nums = process_count + MAGIC_NUM;
    int nums_send[count_nums];

    printf("Rank %d: ", rank);
    for (size_t i = 0; i < count_nums; i++)
    {
        nums_send[i] = i * rank;
        printf("%d, ", nums_send[i]);
    }
    printf("\n");

    MPI_Barrier(MPI_COMM_WORLD);
    
    int nums_recv[count_nums];

    MPI_Reduce(
        &nums_send,
        &nums_recv,
        count_nums,
        MPI_INT,
        MPI_SUM,
        0,
        MPI_COMM_WORLD
    );

    if (rank == 0)
    {   
        printf("Sum     ");
        for (size_t i = 0; i < count_nums; i++)
        {
            printf("%d ", 
                nums_recv[i]);
        }
        printf("\n");
    }

    MPI_Finalize();
    return 0;
}
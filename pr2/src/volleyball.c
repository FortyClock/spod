// Автор: Марк Пархоменко
// Дата: 04.11.2026

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <mpi.h>

int choose_dest(int rank, int process_count)
{
    int dest;

    do
    {
        dest = rand() % process_count;
    }
    while (dest == rank);

    return dest;
}

int main(int argc, char *argv[])
{
    enum
    {
        TAG_BALL = 1,
        TAG_STOP = 2
    };

    int rank;
    int process_count;
    int ball;
    int const max_passes = 10;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &process_count);

    srand((unsigned int)time(NULL) + (unsigned int)rank);

    if (process_count < 2)
    {
        printf("Must have 2 processes\n");
        MPI_Finalize();
        return 0;
    }

    if (rank == 0)
    {
        ball = 1;
        int dest = choose_dest(rank, process_count);
        MPI_Send(
            &ball,
            1,
            MPI_INT,
            dest,
            TAG_BALL,
            MPI_COMM_WORLD
        );
        printf("rank %d, send ball to rank %d\n",
                rank, dest);

    }

    bool running = true;

    while(running)
    {
        int received_msg;
        MPI_Status status;
        MPI_Recv(
            &received_msg,
            1,
            MPI_INT,
            MPI_ANY_SOURCE,
            MPI_ANY_TAG,
            MPI_COMM_WORLD,
            &status
        );

        if (status.MPI_TAG == TAG_BALL)
        {
            printf("Sender %d Receiver %d ball %d\n",
                    status.MPI_SOURCE, rank, received_msg);

            if (received_msg == max_passes)
            {
                for (int i = 0; i < process_count; i++)
                {
                    if (i == rank)
                        continue;
                    MPI_Send(
                        &received_msg,
                        1,
                        MPI_INT,
                        i,
                        TAG_STOP,
                        MPI_COMM_WORLD
                    );
                }
                running = false;

            }
            else
            {
                received_msg++;
                int dest = choose_dest(rank, process_count);
                MPI_Send(
                    &received_msg,
                    1,
                    MPI_INT,
                    dest,
                    TAG_BALL,
                    MPI_COMM_WORLD
                );
            }

        }
        else if (status.MPI_TAG == TAG_STOP)
        {
            printf("The game was stopped, MPI_TAG %d\n",
                    status.MPI_TAG);
            running = false;
        }
    }

    MPI_Finalize();

    return 0;
}

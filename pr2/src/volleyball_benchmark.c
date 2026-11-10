// Автор: Марк Пархоменко
// Экспериментальная версия программы «Пляжный волейбол».

#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mpi.h>

enum
{
    TAG_BALL = 1,
    TAG_STOP = 2
};

static volatile int benchmark_sink;

static int choose_dest(int rank, int process_count)
{
    int dest;

    do
    {
        dest = rand() % process_count;
    }
    while (dest == rank);

    return dest;
}

static bool parse_positive_int(const char *text, int *value)
{
    char *end = NULL;
    errno = 0;
    long parsed = strtol(text, &end, 10);

    if (errno != 0 || end == text || *end != '\0' ||
        parsed <= 0 || parsed > INT_MAX)
    {
        return false;
    }

    *value = (int)parsed;
    return true;
}

static double run_local_round(
    int *message,
    int *scratch,
    int element_count,
    int passes
)
{
    int *source = message;
    int *destination = scratch;
    size_t message_bytes = (size_t)element_count * sizeof(*message);

    MPI_Barrier(MPI_COMM_WORLD);
    double started_at = MPI_Wtime();

    for (int pass = 1; pass <= passes; pass++)
    {
        source[0] = pass;
        memcpy(destination, source, message_bytes);

        int *temporary = source;
        source = destination;
        destination = temporary;
    }

    MPI_Barrier(MPI_COMM_WORLD);
    double elapsed = MPI_Wtime() - started_at;

    benchmark_sink = source[0] + source[element_count - 1];
    return elapsed;
}

static double run_mpi_round(
    int *message,
    int element_count,
    int passes,
    int rank,
    int process_count
)
{
    MPI_Barrier(MPI_COMM_WORLD);
    double started_at = MPI_Wtime();

    if (rank == 0)
    {
        message[0] = 1;
        int destination = choose_dest(rank, process_count);
        MPI_Send(
            message,
            element_count,
            MPI_INT,
            destination,
            TAG_BALL,
            MPI_COMM_WORLD
        );
    }

    bool running = true;
    while (running)
    {
        MPI_Status status;
        MPI_Recv(
            message,
            element_count,
            MPI_INT,
            MPI_ANY_SOURCE,
            MPI_ANY_TAG,
            MPI_COMM_WORLD,
            &status
        );

        if (status.MPI_TAG == TAG_STOP)
        {
            running = false;
        }
        else if (status.MPI_TAG == TAG_BALL)
        {
            if (message[0] >= passes)
            {
                int stop_value = 0;
                for (int destination = 0;
                     destination < process_count;
                     destination++)
                {
                    if (destination != rank)
                    {
                        MPI_Send(
                            &stop_value,
                            1,
                            MPI_INT,
                            destination,
                            TAG_STOP,
                            MPI_COMM_WORLD
                        );
                    }
                }
                running = false;
            }
            else
            {
                message[0]++;
                int destination = choose_dest(rank, process_count);
                MPI_Send(
                    message,
                    element_count,
                    MPI_INT,
                    destination,
                    TAG_BALL,
                    MPI_COMM_WORLD
                );
            }
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);
    double elapsed = MPI_Wtime() - started_at;

    benchmark_sink = message[0];
    return elapsed;
}

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);

    int rank;
    int process_count;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &process_count);

    if (argc != 5)
    {
        if (rank == 0)
        {
            fprintf(
                stderr,
                "Usage: %s PAYLOAD_INTS PASSES REPETITIONS WARMUPS\n",
                argv[0]
            );
        }
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    int payload_count;
    int passes;
    int repetitions;
    int warmups;
    bool arguments_valid =
        parse_positive_int(argv[1], &payload_count) &&
        parse_positive_int(argv[2], &passes) &&
        parse_positive_int(argv[3], &repetitions) &&
        parse_positive_int(argv[4], &warmups) &&
        payload_count < INT_MAX;

    if (!arguments_valid)
    {
        if (rank == 0)
        {
            fprintf(stderr, "All arguments must be positive integers.\n");
        }
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    int element_count = payload_count + 1;
    size_t allocation_size = (size_t)element_count * sizeof(int);
    int *message = malloc(allocation_size);
    int *scratch = process_count == 1 ? malloc(allocation_size) : NULL;

    if (message == NULL || (process_count == 1 && scratch == NULL))
    {
        fprintf(stderr, "Process %d: unable to allocate message buffer.\n", rank);
        free(message);
        free(scratch);
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        return EXIT_FAILURE;
    }

    message[0] = 0;
    for (int index = 1; index < element_count; index++)
    {
        message[index] = index;
    }
    if (scratch != NULL)
    {
        memcpy(scratch, message, allocation_size);
    }

    srand(1234567u + (unsigned int)rank * 104729u);

    int total_rounds = warmups + repetitions;
    for (int round = 0; round < total_rounds; round++)
    {
        double local_elapsed;
        if (process_count == 1)
        {
            local_elapsed = run_local_round(
                message,
                scratch,
                element_count,
                passes
            );
        }
        else
        {
            local_elapsed = run_mpi_round(
                message,
                element_count,
                passes,
                rank,
                process_count
            );
        }

        double maximum_elapsed = 0.0;
        MPI_Reduce(
            &local_elapsed,
            &maximum_elapsed,
            1,
            MPI_DOUBLE,
            MPI_MAX,
            0,
            MPI_COMM_WORLD
        );

        if (rank == 0 && round >= warmups)
        {
            int measured_round = round - warmups + 1;
            const char *mode = process_count == 1 ? "local_copy" : "mpi";
            printf(
                "%d,%zu,%d,%d,%d,%.9f,%s\n",
                payload_count,
                (size_t)payload_count * sizeof(int),
                process_count,
                passes,
                measured_round,
                maximum_elapsed,
                mode
            );
        }
    }

    free(scratch);
    free(message);
    MPI_Finalize();
    return EXIT_SUCCESS;
}

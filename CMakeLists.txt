cmake_minimum_required(VERSION 3.16)

project(spod_practicals LANGUAGES C)

# clangd uses this database for go-to-definition, hover information and
# parameter hints.  Keep it enabled for every CMake configuration.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(MPI REQUIRED COMPONENTS C)

add_executable(task1_unordered pr1/src/task1_unordered.c)
add_executable(task1_benchmark pr1/src/task1_benchmark.c)
add_executable(task2_ordered pr1/src/task2_ordered.c)
add_executable(task1_unordered_benchmark EXCLUDE_FROM_ALL
    pr1/src/task1_unordered_benchmark.c)
add_executable(task2_ordered_benchmark EXCLUDE_FROM_ALL
    pr1/src/task2_ordered_benchmark.c)
add_executable(task2_benchmark pr1/src/task2_benchmark.c)
add_executable(volleyball pr2/src/volleyball.c)
add_executable(volleyball_benchmark pr2/src/volleyball_benchmark.c)

target_compile_features(task1_unordered PRIVATE c_std_11)
target_compile_features(task1_benchmark PRIVATE c_std_11)
target_compile_features(task2_ordered PRIVATE c_std_11)
target_compile_features(task1_unordered_benchmark PRIVATE c_std_11)
target_compile_features(task2_ordered_benchmark PRIVATE c_std_11)
target_compile_features(task2_benchmark PRIVATE c_std_11)
target_compile_features(volleyball PRIVATE c_std_11)
target_compile_features(volleyball_benchmark PRIVATE c_std_11)

target_link_libraries(task1_unordered PRIVATE MPI::MPI_C)
target_link_libraries(task1_benchmark PRIVATE MPI::MPI_C)
target_link_libraries(task2_ordered PRIVATE MPI::MPI_C)
target_link_libraries(task1_unordered_benchmark PRIVATE MPI::MPI_C)
target_link_libraries(task2_ordered_benchmark PRIVATE MPI::MPI_C)
target_link_libraries(task2_benchmark PRIVATE MPI::MPI_C)
target_link_libraries(volleyball PRIVATE MPI::MPI_C)
target_link_libraries(volleyball_benchmark PRIVATE MPI::MPI_C)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(task1_unordered PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(task1_benchmark PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(task2_ordered PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(task1_unordered_benchmark PRIVATE
        -Wall -Wextra -Wpedantic)
    target_compile_options(task2_ordered_benchmark PRIVATE
        -Wall -Wextra -Wpedantic)
    target_compile_options(task2_benchmark PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(volleyball PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(volleyball_benchmark PRIVATE -Wall -Wextra -Wpedantic)
elseif(MSVC)
    target_compile_options(task1_unordered PRIVATE /W4)
    target_compile_options(task1_benchmark PRIVATE /W4)
    target_compile_options(task2_ordered PRIVATE /W4)
    target_compile_options(task1_unordered_benchmark PRIVATE /W4)
    target_compile_options(task2_ordered_benchmark PRIVATE /W4)
    target_compile_options(task2_benchmark PRIVATE /W4)
    target_compile_options(volleyball PRIVATE /W4)
    target_compile_options(volleyball_benchmark PRIVATE /W4)
endif()

# The list is configurable because the useful process counts depend on the
# machine. For example: -DSPOD_PROCESS_COUNTS="1;2;4;8;16"
set(SPOD_PROCESS_COUNTS "1;2;4;8" CACHE STRING
    "Process counts used by MPI run targets and CTest")

# Open MPI counts physical cores as slots by default. This flag also makes
# the available hardware threads usable as slots. Additional flags can be
# supplied as a semicolon-separated list.
set(SPOD_MPIEXEC_PREFLAGS "--use-hwthread-cpus" CACHE STRING
    "Additional flags passed to the MPI launcher")

set(SPOD_BENCHMARK_MESSAGE_SIZE "1024" CACHE STRING
    "Number of integers sent by each process in benchmark runs")

set(_spod_task1_run_targets)
set(_spod_task2_run_targets)
set(_spod_task1_benchmark_run_targets)
set(_spod_task2_benchmark_run_targets)
foreach(process_count IN LISTS SPOD_PROCESS_COUNTS)
    if(NOT process_count MATCHES "^[1-9][0-9]*$")
        message(FATAL_ERROR
            "SPOD_PROCESS_COUNTS must contain positive integers: ${process_count}")
    endif()

    set(run_target "run_task1_p${process_count}")
    add_custom_target(${run_target}
        COMMAND
            ${MPIEXEC_EXECUTABLE}
            ${MPIEXEC_NUMPROC_FLAG} ${process_count}
            ${MPIEXEC_PREFLAGS}
            ${SPOD_MPIEXEC_PREFLAGS}
            $<TARGET_FILE:task1_unordered>
            ${MPIEXEC_POSTFLAGS}
        DEPENDS task1_unordered
        USES_TERMINAL
        VERBATIM
        COMMENT "Running task 1 with ${process_count} MPI processes"
    )
    list(APPEND _spod_task1_run_targets ${run_target})

    set(run_target "run_task2_p${process_count}")
    add_custom_target(${run_target}
        COMMAND
            ${MPIEXEC_EXECUTABLE}
            ${MPIEXEC_NUMPROC_FLAG} ${process_count}
            ${MPIEXEC_PREFLAGS}
            ${SPOD_MPIEXEC_PREFLAGS}
            $<TARGET_FILE:task2_ordered>
            ${MPIEXEC_POSTFLAGS}
        DEPENDS task2_ordered
        USES_TERMINAL
        VERBATIM
        COMMENT "Running task 2 with ${process_count} MPI processes"
    )
    list(APPEND _spod_task2_run_targets ${run_target})

    set(run_target "run_task1_benchmark_p${process_count}")
    add_custom_target(${run_target}
        COMMAND
            ${MPIEXEC_EXECUTABLE}
            ${MPIEXEC_NUMPROC_FLAG} ${process_count}
            ${MPIEXEC_PREFLAGS}
            ${SPOD_MPIEXEC_PREFLAGS}
            $<TARGET_FILE:task1_unordered_benchmark>
            ${MPIEXEC_POSTFLAGS}
            ${SPOD_BENCHMARK_MESSAGE_SIZE}
        DEPENDS task1_unordered_benchmark
        USES_TERMINAL
        VERBATIM
        COMMENT "Running task 1 benchmark with ${process_count} MPI processes"
    )
    list(APPEND _spod_task1_benchmark_run_targets ${run_target})

    set(run_target "run_task2_benchmark_p${process_count}")
    add_custom_target(${run_target}
        COMMAND
            ${MPIEXEC_EXECUTABLE}
            ${MPIEXEC_NUMPROC_FLAG} ${process_count}
            ${MPIEXEC_PREFLAGS}
            ${SPOD_MPIEXEC_PREFLAGS}
            $<TARGET_FILE:task2_ordered_benchmark>
            ${MPIEXEC_POSTFLAGS}
            ${SPOD_BENCHMARK_MESSAGE_SIZE}
        DEPENDS task2_ordered_benchmark
        USES_TERMINAL
        VERBATIM
        COMMENT "Running task 2 benchmark with ${process_count} MPI processes"
    )
    list(APPEND _spod_task2_benchmark_run_targets ${run_target})
endforeach()

add_custom_target(run_task1_all DEPENDS ${_spod_task1_run_targets})
add_custom_target(run_task2_all DEPENDS ${_spod_task2_run_targets})
add_custom_target(run_task1_benchmark_all
    DEPENDS ${_spod_task1_benchmark_run_targets})
add_custom_target(run_task2_benchmark_all
    DEPENDS ${_spod_task2_benchmark_run_targets})

include(CTest)
if(BUILD_TESTING)
    foreach(process_count IN LISTS SPOD_PROCESS_COUNTS)
        add_test(
            NAME task1_p${process_count}
            COMMAND
                ${MPIEXEC_EXECUTABLE}
                ${MPIEXEC_NUMPROC_FLAG} ${process_count}
                ${MPIEXEC_PREFLAGS}
                ${SPOD_MPIEXEC_PREFLAGS}
                $<TARGET_FILE:task1_unordered>
                ${MPIEXEC_POSTFLAGS}
        )
        set_tests_properties(task1_p${process_count} PROPERTIES
            PROCESSORS ${process_count}
            TIMEOUT 30
        )

        add_test(
            NAME task2_p${process_count}
            COMMAND
                ${MPIEXEC_EXECUTABLE}
                ${MPIEXEC_NUMPROC_FLAG} ${process_count}
                ${MPIEXEC_PREFLAGS}
                ${SPOD_MPIEXEC_PREFLAGS}
                $<TARGET_FILE:task2_ordered>
                ${MPIEXEC_POSTFLAGS}
        )
        set_tests_properties(task2_p${process_count} PROPERTIES
            PROCESSORS ${process_count}
            TIMEOUT 30
        )
    endforeach()
endif()

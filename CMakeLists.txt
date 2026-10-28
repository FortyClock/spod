cmake_minimum_required(VERSION 3.16)

project(spod_practicals LANGUAGES C)

find_package(MPI REQUIRED COMPONENTS C)

add_executable(task1_unordered pr1/src/task1_unordered.c)
add_executable(task2_ordered pr1/src/task2_ordered.c)

target_compile_features(task1_unordered PRIVATE c_std_11)
target_compile_features(task2_ordered PRIVATE c_std_11)

target_link_libraries(task1_unordered PRIVATE MPI::MPI_C)
target_link_libraries(task2_ordered PRIVATE MPI::MPI_C)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(task1_unordered PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(task2_ordered PRIVATE -Wall -Wextra -Wpedantic)
elseif(MSVC)
    target_compile_options(task1_unordered PRIVATE /W4)
    target_compile_options(task2_ordered PRIVATE /W4)
endif()

# The list is configurable because the useful process counts depend on the
# machine. For example: -DSPOD_PROCESS_COUNTS="1;2;4;8;16"
set(SPOD_PROCESS_COUNTS "1;2;4;8" CACHE STRING
    "Process counts used by MPI run targets and CTest")

# Open MPI counts physical cores as slots by default. This flag makes the
# hardware threads available as slots as well (12 slots on a 6-core/12-thread
# processor). Additional flags can be supplied as a semicolon-separated list.
set(SPOD_MPIEXEC_PREFLAGS "--use-hwthread-cpus" CACHE STRING
    "Additional flags passed to the MPI launcher")

set(_spod_task1_run_targets)
set(_spod_task2_run_targets)
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
endforeach()

add_custom_target(run_task1_all DEPENDS ${_spod_task1_run_targets})
add_custom_target(run_task2_all DEPENDS ${_spod_task2_run_targets})

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

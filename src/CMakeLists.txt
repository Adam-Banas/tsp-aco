add_library(
    utils SHARED
    Utils.cpp
)

# TODO: Add no-CUDA configuration, so that CPU project can be compiled on a system
# without CUDA support
add_library(
    aco_algorithm SHARED
    AcoAlgorithmCpu.cpp
    AcoAlgorithmGpu.cu
    AcoAlgorithm.cpp
    AcoGraph.cpp
)

target_link_libraries(
    aco_algorithm
    PUBLIC utils
)

add_executable(
    tcp_aco
    main.cpp
)

target_link_libraries(
    tcp_aco
    aco_algorithm
)

add_subdirectory(tests)
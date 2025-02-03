add_library(
    utils SHARED
    Utils.cpp
)

add_library(
    aco_algorithm SHARED
    AcoAlgorithmCpu.cpp
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
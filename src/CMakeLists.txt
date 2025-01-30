add_library(
    aco_algorithm SHARED
    AcoAlgorithm.cpp
    AcoGraph.cpp)

add_executable(
    tcp_aco
    main.cpp
)

add_subdirectory(tests)
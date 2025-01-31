add_library(
    aco_algorithm SHARED
    AcoAlgorithm.cpp
    AcoGraph.cpp)

add_executable(
    tcp_aco
    main.cpp
)

target_link_libraries(
    tcp_aco
    aco_algorithm
)

add_subdirectory(tests)
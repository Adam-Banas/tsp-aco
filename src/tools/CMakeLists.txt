add_executable(
    generate_graph
    generate_graph.cpp
)

target_link_libraries(
    generate_graph
    aco_algorithm
)
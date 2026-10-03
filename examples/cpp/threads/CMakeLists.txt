find_package(Threads REQUIRED)
add_executable(example_cpp_threads main.cpp)
target_link_libraries(example_cpp_threads PRIVATE Threads::Threads)

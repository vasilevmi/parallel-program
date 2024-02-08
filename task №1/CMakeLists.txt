cmake_minimum_required(VERSION 3.10)
project(Zad1)

set(CMAKE_CXX_STANDARD 11)

add_executable(program_float main.cpp)


add_executable(program_double main.cpp)
target_compile_definitions(program_double PRIVATE DOUBLE)
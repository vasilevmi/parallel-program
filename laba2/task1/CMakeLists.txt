cmake_minimum_required(VERSION 3.10)
project(Matrix)

set(CMAKE_CXX_STANDARD 11)

find_package(OpenMP REQUIRED)

add_executable(matrixmult main.cpp)
target_link_libraries(matrixmult PUBLIC OpenMP::OpenMP_CXX)
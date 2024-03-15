cmake_minimum_required(VERSION 3.10)
project(ParallelComputing)

set(CMAKE_CXX_STANDARD 11)
find_package(OpenMP REQUIRED)

add_executable(parallel_computing main.cpp)

if(OpenMP_CXX_FOUND)
    target_link_libraries(parallel_computing PUBLIC OpenMP::OpenMP_CXX)
endif()
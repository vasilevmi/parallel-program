cmake_minimum_required(VERSION 3.10)
project(IntegrationWithOpenMP)

set(CMAKE_C_COMPILER gcc)

add_executable(integration main.c)
target_compile_options(integration PRIVATE -fopenmp -lm)
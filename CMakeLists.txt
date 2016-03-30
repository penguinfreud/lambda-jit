cmake_minimum_required(VERSION 3.4)
project(jit)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.cpp main.h)
add_executable(jit ${SOURCE_FILES})
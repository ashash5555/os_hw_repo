cmake_minimum_required(VERSION 3.15)
project(hw4)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++11 -Wall -pedantic -Werror -fPIC")

add_executable(hw4 test_realloc.cpp
        )
cmake_minimum_required(VERSION 3.15)
project(gnunetpp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(gnunetpp)

option(GNUNETPP_BUILD_EXAMPLES "Build examples" ON)
if(GNUNETPP_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
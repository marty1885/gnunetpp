cmake_minimum_required(VERSION 3.15)
project(gnunetpp)

add_subdirectory(gnunetpp)

option(GNUNETPP_BUILD_EXAMPLES "Build examples" ON)
if(GNUNETPP_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
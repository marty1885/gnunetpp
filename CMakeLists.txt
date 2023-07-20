cmake_minimum_required(VERSION 3.15)
project(gnunetpp CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
find_package(GNUnet REQUIRED)

add_subdirectory(gnunetpp)

option(GNUNETPP_BUILD_EXAMPLES "Build examples" ON)
if(GNUNETPP_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

option(GNUNETPP_BUILD_TESTS "Build tests" OFF)
if(GNUNETPP_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

include(CMakePackageConfigHelpers)
configure_package_config_file(
    gnunetppConfig.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/gnunetppConfig.cmake
    INSTALL_DESTINATION  lib/cmake/gnunetpp)


install(FILES ${CMAKE_CURRENT_BINARY_DIR}/gnunetppConfig.cmake
    DESTINATION lib/cmake/gnunetpp)
install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Findidn.cmake
    DESTINATION lib/cmake/gnunetpp)
install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindGNUnet.cmake
    DESTINATION lib/cmake/GNUnet)
install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Findidn.cmake
    DESTINATION lib/cmake/idn)

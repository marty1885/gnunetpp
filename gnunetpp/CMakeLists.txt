add_library(gnunetpp
    gnunetpp.cpp
    gnunetpp-scheduler.cpp
    gnunetpp-crypto.cpp
    gnunetpp-dht.cpp
    gnunetpp-fs.cpp
    inner/Infra.cpp)
target_include_directories(gnunetpp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
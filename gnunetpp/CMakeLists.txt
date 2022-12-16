add_library(gnunetpp
    gnunetpp.cpp
    gnunetpp-scheduler.cpp
    gnunetpp-crypto.cpp
    gnunetpp-dht.cpp
    gnunetpp-fs.cpp
    gnunetpp-identity.cpp
    gnunetpp-gns.cpp
    gnunetpp-nse.cpp
    gnunetpp-cadet.cpp
    inner/Infra.cpp)
target_include_directories(gnunetpp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_precompile_headers(gnunetpp PRIVATE pch.hpp)
target_link_libraries(gnunetpp PRIVATE idn cppcoro)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DESTINATION include
    FILES_MATCHING PATTERN "*.hpp")
install(
    TARGETS gnunetpp
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include)
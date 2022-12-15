# Speed up build by precompiling headers
add_library(example_pch INTERFACE)
target_precompile_headers(example_pch INTERFACE pch.hpp)

add_executable(gnunetpp-dht dht/main.cpp)
target_link_libraries(gnunetpp-dht gnunetdht gnunetcore gnunetutil gnunetpp example_pch)

add_executable(gnunetpp-fs fs/main.cpp)
target_link_libraries(gnunetpp-fs gnunetfs gnunetcore gnunetidentity gnunetutil gnunetpp example_pch)

add_executable(gnunetpp-identity identity/main.cpp)
target_link_libraries(gnunetpp-identity gnunetidentity gnunetcore gnunetutil gnunetpp example_pch)

add_executable(gnunetpp-scheduler scheduler/main.cpp)
target_link_libraries(gnunetpp-scheduler gnunetcore gnunetutil gnunetpp example_pch)

add_executable(gnunetpp-gns gns/main.cpp)
target_link_libraries(gnunetpp-gns gnunetgns gnunetgnsrecord gnunetcore gnunetutil gnunetpp example_pch)

add_executable(gnunetpp-nse nse/main.cpp)
target_link_libraries(gnunetpp-nse gnunetnse gnunetcore gnunetutil gnunetpp example_pch)

add_executable(gnunetpp-cadet cadet/main.cpp)
target_link_libraries(gnunetpp-cadet gnunetcadet gnunetcore gnunetutil gnunetpp example_pch)
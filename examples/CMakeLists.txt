# Speed up build by precompiling headers
add_library(example_pch INTERFACE)
target_precompile_headers(example_pch INTERFACE pch.hpp)

add_executable(gnunetpp-dht dht/main.cpp)
target_link_libraries(gnunetpp-dht gnunetpp example_pch)

add_executable(gnunetpp-fs fs/main.cpp)
target_link_libraries(gnunetpp-fs gnunetpp example_pch)

add_executable(gnunetpp-identity identity/main.cpp)
target_link_libraries(gnunetpp-identity gnunetpp example_pch)

add_executable(gnunetpp-scheduler scheduler/main.cpp)
target_link_libraries(gnunetpp-scheduler gnunetpp example_pch)

add_executable(gnunetpp-gns gns/main.cpp)
target_link_libraries(gnunetpp-gns gnunetpp example_pch)

add_executable(gnunetpp-nse nse/main.cpp)
target_link_libraries(gnunetpp-nse gnunetpp example_pch)

add_executable(gnunetpp-cadet cadet/main.cpp)
target_link_libraries(gnunetpp-cadet gnunetpp example_pch)

add_executable(gnunetpp-peerinfo peerinfo/main.cpp)
target_link_libraries(gnunetpp-peerinfo gnunetpp example_pch)

add_executable(gnunetpp-datastore datastore/main.cpp)
target_link_libraries(gnunetpp-datastore gnunetpp example_pch)

add_executable(gnunetpp-namestore namestore/main.cpp)
target_link_libraries(gnunetpp-namestore gnunetpp example_pch)

add_executable(gnunetpp-messenger messenger/main.cpp)
target_link_libraries(gnunetpp-messenger gnunetpp example_pch)
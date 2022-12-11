add_executable(gnunetpp-dht dht/main.cpp)
target_link_libraries(gnunetpp-dht gnunetdht gnunetcore gnunetutil gnunetpp)

add_executable(gnunetpp-fs fs/main.cpp)
target_link_libraries(gnunetpp-fs gnunetfs gnunetcore gnunetidentity gnunetutil gnunetpp)

add_executable(gnunetpp-identity identity/main.cpp)
target_link_libraries(gnunetpp-identity gnunetidentity gnunetcore gnunetutil gnunetpp)

add_executable(gnunetpp-scheduler scheduler/main.cpp)
target_link_libraries(gnunetpp-scheduler gnunetcore gnunetutil gnunetpp)

add_executable(gnunetpp-gns gns/main.cpp)
target_link_libraries(gnunetpp-gns gnunetgns gnunetgnsrecord gnunetcore gnunetutil gnunetpp)

add_executable(gnunetpp-nse nse/main.cpp)
target_link_libraries(gnunetpp-nse gnunetnse gnunetcore gnunetutil gnunetpp)
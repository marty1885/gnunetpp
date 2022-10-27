add_executable(gnunetpp-dht dht/main.cpp)
target_link_libraries(gnunetpp-dht gnunetdht gnunetcore gnunetutil gnunetpp)

add_executable(gnunetpp-fs fs/main.cpp)
target_link_libraries(gnunetpp-fs gnunetfs gnunetcore gnunetidentity gnunetutil gnunetpp)

add_executable(gnunetpp-identity identity/main.cpp)
target_link_libraries(gnunetpp-identity gnunetidentity gnunetcore gnunetutil gnunetpp)
add_executable(gunetpp-dht dht/main.cpp)
target_link_libraries(gunetpp-dht gnunetdht gnunetcore gnunetutil gnunetpp)
find_package(Drogon REQUIRED)

add_executable(gnunetpp-selftest test.cpp)
target_link_libraries(gnunetpp-selftest PRIVATE Drogon::Drogon gnunetpp gnunetutil)
target_precompile_headers(gnunetpp-selftest PRIVATE pch.hpp)

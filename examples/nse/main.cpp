#include "CLI/App.hpp"
#include "CLI/Formatter.hpp"
#include "CLI/Config.hpp"

#include "gnunetpp-nse.hpp"
#include "gnunetpp.hpp"

#include <iostream>

using namespace gnunetpp;

std::string right_pad(std::string s, size_t n)
{
    if (s.size() < n)
        s += std::string(n - s.size(), ' ');
    return s;
}

Task<> service(const GNUNET_CONFIGURATION_Handle* cfg)
{
    auto nse = std::make_unique<NSE>(cfg);
    auto [network_size, log2_stddev] = co_await nse->estimate();
    std::cout 
        << "-----------------------------------------\n"
        << "estimated network size     |  log2 stddev\n"
        << "-----------------------------------------\n"
        << right_pad(std::to_string(network_size), 27) << "|  " << log2_stddev << std::endl;
    gnunetpp::shutdown();
}

int main(int argc, char** argv)
{
    CLI::App app("Estimates GNUnet netwrok size using the Network Size Estimator", "gnunetpp-nse");
    CLI11_PARSE(app, argc, argv);

    gnunetpp::start(service);
    return 0;
}
#include <gnunetpp.hpp>
#include <iostream>

void service(const GNUNET_CONFIGURATION_Handle* cfg)
{
    std::cout << "Hello world will be printed in 5 seconds" << std::endl;
    gnunetpp::scheduler::runLater(std::chrono::seconds(5), []() {
        std::cout << "Hello, world!" << std::endl;
        gnunetpp::shutdown();
    });
}

int main()
{
    gnunetpp::run(service);
}
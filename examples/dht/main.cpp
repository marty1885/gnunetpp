#include <gnunetpp.hpp>
#include <iostream>

std::shared_ptr<gnunetpp::DHT> dht;
void service(const GNUNET_CONFIGURATION_Handle* cfg)
{
    dht = std::make_shared<gnunetpp::DHT>(cfg);
    gnunetpp::scheduler::runLater(std::chrono::seconds(11), []{
        gnunetpp::shutdown();
    });
    dht->get("world", [](const std::string_view payload) -> bool {
        std::cout << "Found `" << payload << "`!" << std::endl;
        return true;
    }, std::chrono::seconds(10));
}

int main()
{
    gnunetpp::run(service);
}
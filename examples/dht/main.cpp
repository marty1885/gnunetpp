#include <gnunetpp.hpp>
#include <iostream>

std::shared_ptr<gnunetpp::DHT> dht;
void service(const GNUNET_CONFIGURATION_Handle* cfg)
{
    dht = std::make_shared<gnunetpp::DHT>(cfg);
    gnunetpp::runLater(std::chrono::seconds(1), []{
      gnunetpp::shutdown();
    });
    dht->get("hello", [](const std::string_view payload) -> bool {
        std::cout << "Found " << payload << "!" << std::endl;
        return false;
    }, std::chrono::seconds(10));
    gnunetpp::shutdown();
}

int main()
{
    gnunetpp::run(service);
}
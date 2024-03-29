#include "CLI/App.hpp"
#include "CLI/Formatter.hpp"
#include "CLI/Config.hpp"

#include <gnunetpp.hpp>
#include <iostream>

std::string key;
std::string value;
size_t timeout;
bool run_put;
size_t expiration;
uint32_t replication;

std::shared_ptr<gnunetpp::DHT> dht;
gnunetpp::Task<> service(const GNUNET_CONFIGURATION_Handle* cfg)
{
    // Create a DHT service with a hashtable size of 1 since we only do 1 operation at a time
    dht = std::make_shared<gnunetpp::DHT>(cfg, 1);
    if(run_put) {
        // Put the key/value pair into the DHT
        co_await dht->put(key, value, std::chrono::seconds(expiration), replication);
        std::cout << "Put completed" << std::endl;
        gnunetpp::shutdown();
    }
    
    else {
        // Retrieve value associated with key from the DHT. Be aware that there might be multiple values
        // on the same key. The callback will be called for each value.
        auto iter = dht->get(key, std::chrono::seconds(timeout), GNUNET_BLOCK_TYPE_TEST, replication);
        for(auto it = co_await iter.begin(); it != iter.end(); co_await ++it) {
            std::cout << "Got " << key << " = " << *it << std::endl;
        }
        // Same thing. But using hte callback interface
        // dht->get(key, [](std::string_view payload) -> bool {
        //     std::cout << "Got " << key << " = " << payload << std::endl;

        //     // true here tells GNUNet to keep looking for more values
        //     return true; // return false here to stop looking
        // }, std::chrono::seconds(timeout), GNUNET_BLOCK_TYPE_TEST, replication);
        std::cout << "Get completed" << std::endl;
        gnunetpp::shutdown();
    }
}

int main(int argc, char** argv)
{
    CLI::App app("Interact with GNUnet R5N DHT", "gnunetpp-dht");
    app.require_subcommand(1);
    auto put = app.add_subcommand("put", "Put a key-value pair into the GNUnet DHT");
    auto get = app.add_subcommand("get", "Get a value from the GNUnet DHT");

    put->add_option("-k,--key", key, "Key of the key value pair")->required();
    put->add_option("-v,--value", value, "Value of the key value pair")->required();
    put->add_option("-e,--expiration", expiration, "Expiration time for the value in seconds")->default_val(size_t{3600});
    put->add_option("-r,--replication", replication, "Estimation of how many nearest peer this request reaches "
        "(not data replication count)")->default_val(uint32_t{5});

    get->add_option("-k,--key", key, "Key to look for in  the DHT")->required();
    get->add_option("-t,--timeout", timeout, "Timeout for the DHT search in seconds")->default_val(size_t{10});
    get->add_option("-r,--replication", replication, "Estimation of how many nearest peer this request reaches "
        "(not data replication count)")->default_val(uint32_t{5});

    CLI11_PARSE(app, argc, argv);
    run_put = put->parsed();

    // Run the main service
    gnunetpp::start(service);
}
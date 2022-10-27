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
void service(const GNUNET_CONFIGURATION_Handle* cfg)
{
    // Create a DHT service with a hashtable size of 1 since we only do 1 operation at a time
    dht = std::make_shared<gnunetpp::DHT>(cfg, 1);
    if(run_put) {
        // Put the key/value pair into the DHT
        dht->put(key, value, [] {
            std::cout << "Put completed" << std::endl;
            gnunetpp::shutdown();
        }, std::chrono::seconds(expiration), replication);
    } else {
        // Retrieve value associated with key from the DHT. Be aware that there might be multiple values
        // on the same key. The callback will be called for each value.
        dht->get(key, [](std::string_view payload) -> bool {
            std::cout << "Got " << key << " = " << payload << std::endl;

            // true here tells GNUNet to keep looking for more values
            return true; // return false here to stop looking
        }, std::chrono::seconds(timeout), GNUNET_BLOCK_TYPE_TEST, replication);

        // stop looking for values after timeout. This ends the program
        gnunetpp::scheduler::runLater(std::chrono::seconds(timeout), [] {
            gnunetpp::shutdown();
        });
    }
}

int main(int argc, char** argv)
{
    CLI::App app{"gnunetpp-dht"};
    app.require_subcommand(1);
    auto put = app.add_subcommand("put", "Put a key-value pair into the DHT");
    auto get = app.add_subcommand("get", "Get a value from the DHT");

    put->add_option("-k,--key", key, "Key for the DHT")->required();
    put->add_option("-v,--value", value, "Value for the DHT")->required();
    put->add_option("-e,--expiration", expiration, "Expiration time for the value in seconds")->default_val(size_t{3600});
    put->add_option("-r,--replication", replication, "Estimation of how many nearest peer this request reaches "
        "(not replicating data on DHT)")->default_val(uint32_t{5});

    get->add_option("-k,--key", key, "Key for the DHT")->required();
    get->add_option("-t,--timeout", timeout, "Timeout for the DHT operation in seconds")->default_val(size_t{10});
    get->add_option("-r,--replication", replication, "Estimation of how many nearest peer this request reaches "
        "(not replicating data on DHT)")->default_val(uint32_t{5});

    CLI11_PARSE(app, argc, argv);
    run_put = put->parsed();

    // Run the main service
    gnunetpp::run(service);
}
#include "CLI/App.hpp"
#include "CLI/Formatter.hpp"
#include "CLI/Config.hpp"

#include <gnunetpp.hpp>
#include <gnunetpp-datastore.hpp>
#include <iostream>

std::string key;
std::string value;
size_t timeout;
bool run_put;
size_t expiration;
uint32_t replication;
uint32_t priority;
uint32_t queue_priority;

cppcoro::task<> service(const GNUNET_CONFIGURATION_Handle* cfg)
{
    auto datastore = std::make_shared<gnunetpp::DataStore>(cfg);
    if(run_put) {
        // Store the data into the datastore under the given key. The key can be a string or GNUNET_HashCode
        // The datastore is more like a cache then a real key value store. It has user configurable storage limits.
        // Once the limit is reached, the datastore will start to delete old and low priority data. If the data is
        // not in the datastore anymore and no replication to other peers was available, the data is lost forever.
        // This is useful for storing non-critical data like can be retrieved from the network again if needed.
        co_await datastore->put(key, value, std::chrono::seconds(expiration), priority, replication);
    }
    else {
        // Lookup the data in the datastore under the given key
        auto lookup = datastore->get(key, queue_priority);

        // iterate over all values found and print them to stdout
        size_t count = 0;
        for(auto it = co_await lookup.begin(); it != lookup.end(); co_await ++it) {
            auto& data = *it;
            auto sv = std::string_view((char*)data.data(), data.size());
            std::cout << sv << std::endl;
            count++;
        }

        if(count == 0) {
            std::cerr << "No value found" << std::endl;
        }
    }
    gnunetpp::shutdown();
}

int main(int argc, char** argv)
{
    CLI::App app("Store data in the GNUnet local storage", "gnunetpp-datastore");
    app.require_subcommand(1);
    auto put = app.add_subcommand("put", "Put a key-value pair into the Datastore");
    auto get = app.add_subcommand("get", "Get values from the GNUnet Datastore");

    put->add_option("-k,--key", key, "Key of the key value pair")->required();
    put->add_option("-v,--value", value, "Value of the key value pair")->required();
    put->add_option("-e,--expiration", expiration, "Expiration time for the value in seconds")->default_val(size_t{3600});
    put->add_option("-r,--replication", replication, "The replication factor")->default_val(uint32_t{0});
    put->add_option("-p,--priority", priority, "The priority of the value, determines who gets removed when Datastore is full")->default_val(uint32_t{42});
    

    get->add_option("-k,--key", key, "Key to look for in  the Datastore")->required();
    get->add_option("-q,--queue-priority", queue_priority, "Priority of this request in the queue")->default_val(uint32_t{1});

    CLI11_PARSE(app, argc, argv);
    run_put = put->parsed();

    // Run the main service
    gnunetpp::start(service);
}
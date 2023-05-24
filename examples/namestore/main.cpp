#include "CLI/App.hpp"
#include "CLI/Formatter.hpp"
#include "CLI/Config.hpp"

#include <gnunetpp.hpp>
#include <gnunetpp-namestore.hpp>
#include <iostream>

std::string key;
std::string identity_name;
std::string value;
std::string type = "TXT";
size_t expiration = 1200;
bool publish = false;
bool run_lookup = false;
bool run_store = false;
bool run_remove = false;

cppcoro::task<> service(const GNUNET_CONFIGURATION_Handle* cfg)
{
    auto namestore = std::make_shared<gnunetpp::Namestore>(cfg);

    auto id = co_await gnunetpp::getEgo(cfg, identity_name);
    if (!id) {
        std::cout << "Identity " << identity_name << " not found" << std::endl;
        co_return;
    }

    if(run_lookup) {
        auto records = co_await namestore->lookup(id->privateKey(), key);
        for (auto& record : records)
            std::cout  << record.value << " - " << record.type << std::endl;

        if(records.empty())
            std::cout << "No records found" << std::endl;
    }
    else if (run_store) {
        auto success = co_await namestore->store(id->privateKey(), key, value, type, std::chrono::seconds(expiration), publish);
        if(!success)
            std::cerr << "Failed to store record" << std::endl;
    }
    else if (run_remove) {
        // Remove is always successful as either the record is removed or it was not there in the first place
        co_await namestore->remove(id->privateKey(), key);
    }

    gnunetpp::shutdown();
}

int main(int argc, char** argv)
{
    CLI::App app("Store data in the GNUnet local storage", "gnunetpp-datastore");
    app.require_subcommand(1);
    auto get = app.add_subcommand("get", "Get values from the local namestore");
    auto store = app.add_subcommand("store", "Store a record in the local namestore");
    auto remove = app.add_subcommand("remove", "Remove a record from the local namestore");

    get->add_option("-k,--key", key, "Key to look for in  the Datastore")->required();
    get->add_option("-i,--identity", identity_name, "The identity to use for the lookup")->required();

    store->add_option("-k,--key", key, "Key to store in the Datastore")->required();
    store->add_option("-i,--identity", identity_name, "The identity to use for the lookup")->required();
    store->add_option("-v,--value", value, "The value to store in the Datastore")->required();
    store->add_option("-t,--type", type, "The type of the record to store in the Datastore")->default_str("TXT");
    store->add_option("-e,--expiration", expiration, "The expiration time of the record in seconds")->default_val(1200);
    store->add_flag("-p,--public", publish, "Weather the record is public accessible from other peers")->default_val(false);

    remove->add_option("-k,--key", key, "Key to remove from the Datastore")->required();
    remove->add_option("-i,--identity", identity_name, "The identity to use for the lookup")->required();

    CLI11_PARSE(app, argc, argv);
    run_lookup = get->parsed();
    run_store = store->parsed();
    run_remove = remove->parsed();

    // Run the main service
    gnunetpp::start(service);
}
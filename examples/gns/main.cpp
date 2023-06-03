#include "CLI/App.hpp"
#include "CLI/Formatter.hpp"
#include "CLI/Config.hpp"

#include "gnunetpp-gns.hpp"
#include "gnunetpp-scheduler.hpp"
#include "inner/coroutine.hpp"

std::string name;
std::string record_type;
int timeout;
bool dns_compatability;

gnunetpp::Task<> service(const GNUNET_CONFIGURATION_Handle* cfg)
{
    auto gns = std::make_shared<gnunetpp::GNS>(cfg);
    auto result = co_await gns->lookup(name, std::chrono::seconds(timeout), record_type, dns_compatability);
    if(result.empty())
        std::cout << "No results found under domain name: " << name << std::endl;
    for (const auto& [value, type] : result)
        std::cout << value << " - " << type << std::endl;
    gnunetpp::shutdown();
}

int main(int argc, char** argv)
{
    CLI::App app{"Name resolution usign GNS", "gnunetpp-gns"};

    app.add_option("-t,--timeout", timeout, "Timeout in seconds")->default_val(10);
    app.add_option("name", name, "Name to lookup")->required();
    app.add_option("--type", record_type, "Record type to lookup (Ex. A, AAAA)")->default_val("ANY");
    app.add_flag("--dns", dns_compatability, "Lookup DNS if not found in GNS")->default_val(false);

    CLI11_PARSE(app, argc, argv);
    gnunetpp::start(service);

    return 0;
}


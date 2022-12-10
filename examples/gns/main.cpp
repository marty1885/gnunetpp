#include "CLI/App.hpp"
#include "CLI/Formatter.hpp"
#include "CLI/Config.hpp"

#include "gnunetpp-gns.hpp"

std::string name;
std::string record_type;
int timeout;

std::shared_ptr<gnunetpp::GNS> gns;
void service(const GNUNET_CONFIGURATION_Handle* cfg)
{
    gns = std::make_shared<gnunetpp::GNS>(cfg);
    gns->lookup(name, std::chrono::seconds(timeout), [](const std::vector<std::string>& result) {
        if(result.empty()) {
            std::cout << "No results found under domain name: " << name << std::endl;
            gnunetpp::shutdown();
            return;
        }
        for (const auto& r : result)
            std::cout << r << std::endl;
        gnunetpp::shutdown();
    }, record_type);
}

int main(int argc, char** argv)
{
    CLI::App app{"gnunetpp-gns"};

    app.add_option("-t,--timeout", timeout, "Timeout in seconds")->default_val(10);
    app.add_option("name", name, "Name to lookup")->required();
    app.add_option("--type", record_type, "Record type to lookup (Ex. A, AAAA)")->default_val("ANY");

    CLI11_PARSE(app, argc, argv);
    gnunetpp::run(service);

    return 0;
}


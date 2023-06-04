#include "CLI/App.hpp"
#include "CLI/Formatter.hpp"
#include "CLI/Config.hpp"

#include "gnunetpp-peerinfo.hpp"
#include "gnunetpp.hpp"

using namespace gnunetpp;
bool show_hello = false;

Task<> service(const GNUNET_CONFIGURATION_Handle* cfg)
{
    auto peerinfo = std::make_shared<PeerInfo>(cfg);

    if(show_hello) {
        auto hello = co_await helloMessage(cfg, GNUNET_TRANSPORT_AC_ANY);
        std::cout << to_string(hello) << std::endl;
    }
    else {
        // retrieve all peers that we know about
        auto peers = co_await peerinfo->peers();
        std::cout << "Peers:" << std::endl;
        for (auto& peer : peers) {
            std::cout << crypto::to_string(peer) << std::endl;
        }
    }
    gnunetpp::shutdown();
}

int main(int argc, char** argv)
{
    CLI::App app("List about known peers in the network", "gnunetpp-peerinfo");
    auto list = app.add_subcommand("list", "List peers");
    auto hello = app.add_subcommand("hello", "Show the HELLO message of the local peer");
    CLI11_PARSE(app, argc, argv);

    show_hello = hello->parsed();


    gnunetpp::start(service);
    return 0;
}

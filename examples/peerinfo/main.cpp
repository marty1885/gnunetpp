#include "CLI/App.hpp"
#include "CLI/Formatter.hpp"
#include "CLI/Config.hpp"

#include "gnunetpp-peerinfo.hpp"
#include "gnunetpp.hpp"

using namespace gnunetpp;
bool show_hello = false;

Task<> service(const GNUNET_CONFIGURATION_Handle* cfg)
{
    if(show_hello) {
        // retrieve the HELLO message of the local peer, This message can be sent to another node (out of band)
        // to notify it about our existence. The other node can then use this HELLO message to connect to us
        // by calling `co_await peerinfo->addPeer(hello)`
        auto hello = co_await helloMessage(cfg);
        std::cout << to_string(hello) << std::endl;
    }
    else {
        // retrieve all peers that we know about
        auto peerinfo = std::make_shared<PeerInfo>(cfg);
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

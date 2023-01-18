#include "CLI/App.hpp"
#include "CLI/Formatter.hpp"
#include "CLI/Config.hpp"

#include "gnunetpp-peerinfo.hpp"
#include "gnunetpp.hpp"

using namespace gnunetpp;


cppcoro::task<> service(const GNUNET_CONFIGURATION_Handle* cfg)
{
    std::cout << "Peers:" << std::endl;
    auto peerinfo = std::make_shared<PeerInfo>(cfg);
    auto peers = co_await peerinfo->peers();
    for (auto& peer : peers) {
        std::cout << crypto::to_string(peer) << std::endl;
    }
    gnunetpp::shutdown();
}

int main(int argc, char** argv)
{
    CLI::App app("List about known peers in the network", "gnunetpp-peerinfo");
    CLI11_PARSE(app, argc, argv);

    gnunetpp::start(service);
    return 0;
}

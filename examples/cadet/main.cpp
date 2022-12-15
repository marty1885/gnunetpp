#include "gnunetpp-cadet.hpp"
#include "gnunetpp.hpp"

#include <iostream>

using namespace gnunetpp;

cppcoro::task<> service(const GNUNET_CONFIGURATION_Handle* cfg)
{
    auto peers = co_await CADET::list_peers(cfg);

    for (auto& peer : peers)
        std::cout << "Peer: " << crypto::to_string(peer.peer) << " n_path: " << peer.n_paths << " have_tunnel: " << peer.have_tunnel << std::endl;
}

int main()
{
    gnunetpp::start(service);
}
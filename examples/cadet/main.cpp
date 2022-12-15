#include "CLI/App.hpp"
#include "CLI/Formatter.hpp"
#include "CLI/Config.hpp"

#include "gnunetpp-cadet.hpp"
#include "gnunetpp.hpp"

#include <iostream>

using namespace gnunetpp;
using namespace std::chrono_literals;

bool run_peer_list = false;
bool run_server = false;
bool run_client = false;
std::string peer;
std::string port = "default";

cppcoro::task<> service(const GNUNET_CONFIGURATION_Handle* cfg)
{
    if(run_peer_list) {
        auto peers = co_await CADET::list_peers(cfg);

        std::cout << "Current peer ID: " << crypto::to_string(crypto::my_peer_identity(cfg)) << std::endl << std::endl;
        for (auto& peer : peers) {
            std::string path_length = std::to_string(peer.best_path_length);
            // special case: 0 = unknown, 1 = ourselves, 2 = direct neighbour
            if(peer.best_path_length == 0)
                path_length = "unknown";
            else if(peer.best_path_length == 1)
                path_length = "ourselves";
            else if(peer.best_path_length == 2)
                path_length = "direct neighbour";
            std::cout << "Peer: " << crypto::to_string(peer.peer) << " n_path: " << peer.n_paths << " have_tunnel: " << peer.have_tunnel << " path length: " << path_length << std::endl;
        }
    }
    else if(run_client) {
        auto cadet = std::make_shared<CADET>(cfg);
        auto channel = cadet->connect(crypto::peer_identity(peer), port);
        channel->setDisconnectCallback([] { gnunetpp::shutdown(); });
        channel->setReceiveCallback([](const std::string_view data, uint16_t type) {
            std::cout << data << std::flush;
        });

        while(true) {
            auto line = co_await scheduler::read_line();
            channel->send(line.data(), line.size(), GNUNET_MESSAGE_TYPE_CADET_CLI);
        }
    }
    else {
        std::cerr << "Not implemented yet" << std::endl;
        abort();
    }
}

int main(int argc, char** argv)
{
    CLI::App app("Swiss army knife for CADET", "gnunetpp-cadet");

    auto peer_list = app.add_subcommand("list", "List all peers")->callback([&] { run_peer_list = true; });
    auto server = app.add_subcommand("server", "Run an echo server (like nc -l")->callback([&] { run_server = true; });
    auto client = app.add_subcommand("client", "Run an echo client (like nc)")->callback([&] { run_client = true; });
    server->add_option("port", port, "Port to listen on")->default_val("default");
    client->add_option("peer", peer, "Peer to connect to")->required();
    client->add_option("port", port, "Port to connect to")->default_val("default");

    app.require_subcommand(1);
    CLI11_PARSE(app, argc, argv);

    gnunetpp::start(service);
}
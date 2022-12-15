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
bool verbose = false;
std::string peer;
std::string port = "default";

static std::string right_pad(const std::string& str, size_t len)
{
    std::string ret = str;
    ret.resize(len, ' ');
    return ret;
}

static std::string bool_to_string(bool b)
{
    return b ? "true" : "false";
}

cppcoro::task<> service(const GNUNET_CONFIGURATION_Handle* cfg)
{
    if(run_peer_list) {
        // Get a list of all peers currently known over CADET (Different from the list of peers in the peer service)
        auto peers = co_await CADET::list_peers(cfg);

        std::cout << "Current peer ID: " << crypto::to_string(crypto::my_peer_identity(cfg)) << std::endl << std::endl;
        for (auto& peer : peers) {
            std::cout << "Peer: " << crypto::to_string(peer.peer) << " n_path: " << right_pad(std::to_string(peer.n_paths), 4)
                << " have_tunnel: " << bool_to_string(peer.have_tunnel) << std::endl;
        }
    }
    else if(run_client) {
        // Create a CADET instance
        auto cadet = std::make_shared<CADET>(cfg);

        // CADET connections are called channels. We connect to a peer using the following information
        // - The peer identity of the peer we want to connect to
        // - The port we want to connect to
        // - The message types we want to receive
        // Noteice that the port in CADET is a string, not a number. CADET have the concept of "message types" which
        // every message is tagged with. And all message type that the channel is interested in must be specified during
        // channel creation. In this case we are only interested in messages of type GNUNET_MESSAGE_TYPE_CADET_CLI
        //
        // CADET is like a hybrid between TCP and UDP. CADET is a message based protocol, i.e. "packets" are the smallest
        // unit of data. But unlike UDP, CADET supports reliable and ordered messages.
        // By default a CADET channel ordered and reliable but low priority. This means other traffic will be prioritized
        // over this channel. Also "reliable" means "best effort" in this case. Just like TCP.
        // FIXME: GNUnet should support all messages using GNUNET_MESSAGE_TYPE_ALL. But it doesn't work as of 0.19.0
        auto channel = cadet->connect(crypto::peer_identity(peer), port, {GNUNET_MESSAGE_TYPE_CADET_CLI});
        // If you want unreliable and unordered messages, you can use the following:
        // auto channel = cadet->connect(crypto::peer_identity(peer), port, {GNUNET_MESSAGE_TYPE_CADET_CLI}
        //  , GNUNET_MQ_PREF_UNRELIABLE | GNUNET_MQ_PREF_UNORDERED);

        // Setup callbacks for the channel
        channel->setDisconnectCallback([] {
            gnunetpp::shutdown();
        });
        channel->setReceiveCallback([](const std::string_view data, uint16_t type) {
            std::cout << data << std::flush;
        });

        while(true) {
            // Read stdin line by line asynchronously
            auto line = co_await scheduler::read_line();
            // Send the line to the peer with the specified message type
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
    client->add_flag("-v,--verbose", verbose, "Verbose output");

    app.require_subcommand(1);
    CLI11_PARSE(app, argc, argv);

    gnunetpp::start(service);
}
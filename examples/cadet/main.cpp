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
bool run_tunnel = false;
bool list_show_path = false;
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

std::string encryption_status(uint16_t estate)
{
    switch(estate) {
        case (int)CADET::EncryptionState::Uninitialized:
            return "Uninitialized    ";
        case (int)CADET::EncryptionState::AXSent:
            return "AXSent           ";
        case (int)CADET::EncryptionState::AXReceived:
            return "AXReceived       ";
        case (int)CADET::EncryptionState::AxAuxSent:
            return "AxAuxSent        ";
        case (int)CADET::EncryptionState::AXSentAndReceived:
            return "AXSentAndReceived";
        case (int)CADET::EncryptionState::Ok:
            return "Ok               ";
        default:
            return "Unknown          "; // Should never happen
    }
}

std::string connection_status(uint16_t cstate)
{
    switch(cstate) {
        case (int)CADET::ConnectionState::New:
            return "New     ";
        case (int)CADET::ConnectionState::Search:
            return "Search  ";
        case (int)CADET::ConnectionState::Wait:
            return "Wait    ";
        case (int)CADET::ConnectionState::Ready:
            return "Ready   ";
        case (int)CADET::ConnectionState::Shutdown:
            return "Shutdown";
        default:
            return "Unknown "; // Should never happen
    }
}

static std::shared_ptr<CADET> cadet;
Task<> service(const GNUNET_CONFIGURATION_Handle* cfg)
{
    if(run_peer_list) {
        // Get a list of all peers currently known over CADET (Different from the list of peers in the peer service)
        auto peers = co_await CADET::listPeers(cfg);

        std::cout << "Host peer ID: " << crypto::to_string(crypto::myPeerIdentity(cfg)) << std::endl;
        std::cout << "Total peers: " << peers.size() << std::endl << std::endl;
        for (auto& peer : peers) {
            std::cout << "Peer: " << crypto::to_string(peer.peer) << " n_path: " << right_pad(std::to_string(peer.n_paths), 4)
                << " have_tunnel: " << bool_to_string(peer.have_tunnel) << std::endl;
            
            if(!list_show_path)
                continue;
            // Retrieve all paths to the peer. Note that sometimes GNUnet will report more paths in the last step than
            // we will get here. Likely because TOCTOU, information not yet updated, or other reasons. Always assume
            // path information not complete in a p2p network.
            auto paths_to_peer = co_await CADET::pathsToPeer(cfg, peer.peer);
            for(size_t i=0;i<paths_to_peer.size();i++) {
                auto& path = paths_to_peer[i];
                std::cout << "  Path " << i << " has size " << path.size() << ":\n";
                for(auto& hop : path)
                    std::cout << "    " << crypto::to_string(hop) << "\n";
                std::cout << std::endl;
            }
        }
        gnunetpp::shutdown();
    }
    else if(run_client) {
        // Create a CADET instance
        cadet = std::make_shared<CADET>(cfg);

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
        auto channel = cadet->connect(crypto::peerIdentity(peer), port, {GNUNET_MESSAGE_TYPE_CADET_CLI});
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
        
        // A channel we created is "outgoing". This means we are the initiator of the connection. We can check
        // this using the isOutgoing() function. Also isIncoming() shall return false.
        GNUNET_assert(channel->isOutgoing() == true);
        GNUNET_assert(channel->isIncoming() == false);

        while(true) {
            // Read stdin line by line asynchronously
            std::string data;
            try {
                data = co_await scheduler::readStdin();
            }
            catch(const std::exception& e) {
                // If we get an exception, it means the user pressed Ctrl+D. This means we should disconnect
                // channel->disconnect();
                break;
            }
            // Send the line to the peer with the specified message type
            channel->send(data.data(), data.size(), GNUNET_MESSAGE_TYPE_CADET_CLI);
        }
        gnunetpp::shutdown();
    }
    else if (run_server) {
        // Create a CADET instance
        cadet = std::make_shared<CADET>(cfg);
        // Setup a callback for new connections
        cadet->setConnectedCallback([](const CADETChannelPtr& channel) {
            std::cout << "* New connection from " << crypto::to_string(channel->peer()) << std::endl;

            // A channel we received is "incoming". This means we are the receiver of the connection. We can check
            // this using the isIncoming() function. Also isOutgoing() shall return false.
            GNUNET_assert(channel->isIncoming() == true);
            GNUNET_assert(channel->isOutgoing() == false);
        });
        // This function is called when a new message is received
        cadet->setReceiveCallback([](const CADETChannelPtr& channel, const std::string_view data, uint16_t type) {
            std::cout << data << std::flush;
        });
        // This function is called when the connection is closed (either by us or the peer)
        cadet->setDisconnectedCallback([](const CADETChannelPtr& channel) {
            std::cout << "* Connection closed for " << crypto::to_string(channel->peer()) << std::endl;
        });

        // Listen on a port. Again, the port is a string, not a number.
        cadet->openPort(port, {GNUNET_MESSAGE_TYPE_CADET_CLI});
        std::cout << "Listening on " << crypto::to_string(crypto::myPeerIdentity(cfg)) <<" port \'" << port << "\'" << std::endl;
    }
    else if (run_tunnel) {
        // Request list of all tunnels
        auto tunnels = co_await CADET::listTunnels(cfg);
        for(auto& tunnel : tunnels) {
            std::cout << "Tunnel: " << crypto::to_string(tunnel.peer) << " [ ENC: " << encryption_status(tunnel.estate) << ", CON: " << connection_status(tunnel.cstate)  << "] "
                << tunnel.channels << " CHs, " << tunnel.connections << " CONNs" << std::endl;
        }
        gnunetpp::shutdown();
    }
}

int main(int argc, char** argv)
{
    CLI::App app("Swiss army knife for CADET", "gnunetpp-cadet");

    auto peer_list = app.add_subcommand("list", "List all peers")->callback([&] { run_peer_list = true; });
    auto server = app.add_subcommand("server", "Run an echo server (like nc -l")->callback([&] { run_server = true; });
    auto client = app.add_subcommand("client", "Run an echo client (like nc)")->callback([&] { run_client = true; });
    auto tunnel = app.add_subcommand("list-tunnel", "List info about all existing CADET tunnels")->callback([&] { run_tunnel = true; });
    peer_list->add_flag("-p,--path", list_show_path, "Show paths to peers");
    server->add_option("port", port, "Port to listen on")->default_val("default");
    client->add_option("peer", peer, "Peer to connect to")->required();
    client->add_option("port", port, "Port to connect to")->default_val("default");

    app.require_subcommand(1);
    CLI11_PARSE(app, argc, argv);

    gnunetpp::start(service);
}
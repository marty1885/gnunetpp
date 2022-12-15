#include "gnunetpp-cadet.hpp"
#include "gnunetpp-crypto.hpp"
#include <gnunet/gnunet_protocols.h>

#include <iostream>

using namespace gnunetpp;

// Note this function returns a pointer as the closure of the connection
static void *cadet_connection_trampoline (
  void *cls,
  GNUNET_CADET_Channel *channel,
  const GNUNET_PeerIdentity *source)

{
    std::cout << "CADET connection trampoline" << std::endl;
    auto cadet = static_cast<CADET*>(cls);
    GNUNET_assert(cadet->openChannels.find(channel) == cadet->openChannels.end());
    cadet->openChannels[channel] = std::make_unique<CADETChannel>(channel);
    return nullptr;
}

static void cadet_disconnect_trampoline(void *cls, const GNUNET_CADET_Channel *channel)
{
    std::cout << "CADET disconnect trampoline" << std::endl;
    auto cadet = static_cast<CADET*>(cls);
    auto channelPtr = const_cast<GNUNET_CADET_Channel*>(channel);
    GNUNET_assert(cadet->openChannels.find(channelPtr) != cadet->openChannels.end());
    cadet->openChannels.erase(channelPtr);
}

static void cadet_disconnect_client_trampoline(void *cls, const GNUNET_CADET_Channel *channel)
{
    std::cout << "CADET disconnect client trampoline" << std::endl;
    auto channel_ptr = static_cast<CADETChannel*>(cls);
    if(channel_ptr->disconnectCallback)
        channel_ptr->disconnectCallback();
    delete channel_ptr;
}

static int accept_all(void *cls, const struct GNUNET_MessageHeader *msg)
{
    return GNUNET_YES;
}

static void process_message(void *cls, const struct GNUNET_MessageHeader *msg)
{
    std::cout << "CADET process message" << std::endl;
}

CADET::CADET(const GNUNET_CONFIGURATION_Handle* cfg)
{
    cadet = GNUNET_CADET_connect(cfg);
    if(!cadet)
        throw std::runtime_error("Failed to connect to CADET service");
    registerService(this);
}

CADET::~CADET()
{
    shutdown();
    removeService(this);
}

void CADET::shutdown()
{
    if(cadet) {
        GNUNET_CADET_disconnect(cadet);
        cadet = nullptr;
    }
}

GNUNET_CADET_Port* CADET::openPort(const std::string_view port)
{
    // NOTE: Not working yet
    auto hash = crypto::hash(port);
    const GNUNET_MQ_MessageHandler handlers[] = {
        {
            accept_all,
            process_message,
            nullptr,
            GNUNET_MESSAGE_TYPE_ALL,
            0
        },
        GNUNET_MQ_handler_end()
    };
    return GNUNET_CADET_open_port(cadet, &hash, cadet_connection_trampoline, nullptr, nullptr, cadet_disconnect_trampoline, handlers);
}

void CADET::closePort(GNUNET_CADET_Port* port)
{
    GNUNET_CADET_close_port(port);
}

CADETChannel CADET::connect(const std::string_view port, const GNUNET_PeerIdentity& peer)
{
    auto channel_ptr = new CADETChannel();
    auto hash = crypto::hash(port);
    auto channel = GNUNET_CADET_channel_create(cadet, channel_ptr, &peer, &hash, nullptr, cadet_disconnect_client_trampoline, nullptr);
    channel_ptr->channel = channel;
    return CADETChannel(channel);
}

struct ListPeersCallbackPack
{
    std::function<void(const std::vector<GNUNET_CADET_PeerListEntry>&)> callback;
    std::vector<GNUNET_CADET_PeerListEntry> peers;
};

static void list_peers_trampoline(void* cls, const GNUNET_CADET_PeerListEntry *ple)
{
    auto pack = static_cast<ListPeersCallbackPack*>(cls);
    GNUNET_assert(pack != nullptr);
    if(ple)
        pack->peers.push_back(*ple);
    else {
        pack->callback(pack->peers);
        delete pack;
    }
}

void CADET::list_peers(const GNUNET_CONFIGURATION_Handle* cfg, std::function<void(const std::vector<GNUNET_CADET_PeerListEntry>&)> callback)
{
    auto cls = new ListPeersCallbackPack{callback, {}};
    GNUNET_CADET_list_peers(cfg, list_peers_trampoline, cls);
}

cppcoro::task<std::vector<GNUNET_CADET_PeerListEntry>> CADET::list_peers(const GNUNET_CONFIGURATION_Handle* cfg)
{
    struct PeerListAwaiter : public EagerAwaiter<std::vector<GNUNET_CADET_PeerListEntry>> {
        PeerListAwaiter(const GNUNET_CONFIGURATION_Handle* cfg) {
            list_peers(cfg, [this](const std::vector<GNUNET_CADET_PeerListEntry>& peers) {
                setValue(peers);
            });
        }
    };
    co_return co_await PeerListAwaiter(cfg);
}

void CADETChannel::send(const void* data, size_t size, uint16_t type)
{
    const size_t total_size = size + sizeof(struct GNUNET_MessageHeader);
    struct GNUNET_MessageHeader *msg = nullptr;
    auto env = GNUNET_MQ_msg_extra(msg, total_size, type);
    memcpy(&msg[1], data, size);
    GNUNET_MQ_send(getMQ(), env);
}
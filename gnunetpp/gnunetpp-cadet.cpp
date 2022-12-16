#include "gnunetpp-cadet.hpp"
#include "gnunetpp-crypto.hpp"
#include <gnunet/gnunet_protocols.h>

#include <iostream>

using namespace gnunetpp;

struct PortListenerPack
{
    CADET* cadet;
    CADETChannel* channel;
};


// Note this function returns a pointer as the closure of the connection
static void *cadet_connection_trampoline (
    void *cls,
    GNUNET_CADET_Channel *channel,
    const GNUNET_PeerIdentity *source)

{
    auto cadet = static_cast<CADET*>(cls);
    auto pack = new PortListenerPack();
    pack->cadet = cadet;
    pack->channel = new CADETChannel(channel);
    if(cadet->connectedCallback)
        cadet->connectedCallback(pack->channel);
    return pack;
}

static void cadet_disconnect_trampoline(void *cls, const GNUNET_CADET_Channel *channel)
{
    auto portListenerPack = static_cast<PortListenerPack*>(cls);
    auto cadet = portListenerPack->cadet;
    if(portListenerPack->channel->disconnectCallback)
        portListenerPack->channel->disconnectCallback();
    // HACK: GNUnet already destroyed the channel, so we don't want the destructor to try to destroy it again
    portListenerPack->channel->channel = nullptr;
    delete portListenerPack->channel;
    delete portListenerPack;
}

static void cadet_disconnect_client_trampoline(void *cls, const GNUNET_CADET_Channel *channel)
{
    auto channel_ptr = static_cast<CADETChannel*>(cls);
    if(channel_ptr->disconnectCallback)
        channel_ptr->disconnectCallback();
    channel_ptr->channel = nullptr;
    delete channel_ptr;
}

static int accept_all(void *cls, const struct GNUNET_MessageHeader *msg)
{
    return GNUNET_YES;
}

static void cadet_message_trampoline(void *cls, const struct GNUNET_MessageHeader *msg)
{
    auto pack = static_cast<PortListenerPack*>(cls);
    auto size = ntohs(msg->size);
    auto type = ntohs(msg->type);
    auto message_begin = reinterpret_cast<const char*>(msg) + sizeof(GNUNET_MessageHeader);
    auto message_end = message_begin + size - sizeof(GNUNET_MessageHeader);
    if(pack->channel->readCallback)
        pack->channel->readCallback(std::string_view(message_begin, message_end - message_begin), type);
    GNUNET_CADET_receive_done(pack->channel->channel);
}

static void cadet_message_client_trampoline(void *cls, const struct GNUNET_MessageHeader *msg)
{
    std::cout << "CADET process message client" << std::endl;
    auto channel_ptr = static_cast<CADETChannel*>(cls);

    auto size = ntohs(msg->size);
    auto type = ntohs(msg->type);
    auto message_begin = reinterpret_cast<const char*>(msg) + sizeof(GNUNET_MessageHeader);
    auto message_end = message_begin + size - sizeof(GNUNET_MessageHeader);
    if(channel_ptr->readCallback)
        channel_ptr->readCallback(std::string_view(message_begin, message_end - message_begin), type);
    GNUNET_CADET_receive_done(channel_ptr->channel);
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
    const GNUNET_MQ_MessageHandler handlers[] = {
        {
            accept_all,
            cadet_message_trampoline,
            this,
            GNUNET_MESSAGE_TYPE_CADET_CLI,
            0
        },
        GNUNET_MQ_handler_end()
    };

    auto hash = crypto::hash(port);
    return GNUNET_CADET_open_port(cadet, &hash, cadet_connection_trampoline, this, nullptr, cadet_disconnect_trampoline, handlers);
}

void CADET::closePort(GNUNET_CADET_Port* port)
{
    GNUNET_CADET_close_port(port);
}

CADETChannel* CADET::connect(const GNUNET_PeerIdentity& peer, const std::string_view port
    , const std::vector<uint16_t>& acceptable_reply_types
    , std::optional<uint32_t> options)
{
    auto channel_ptr = new CADETChannel();
    auto hash = crypto::hash(port);
    std::vector<GNUNET_MQ_MessageHandler> handlers;
    handlers.reserve(acceptable_reply_types.size() + 1);
    for(auto type : acceptable_reply_types) {
        handlers.push_back({
            accept_all,
            cadet_message_client_trampoline,
            channel_ptr,
            type,
            0
        });
    }
    handlers.push_back(GNUNET_MQ_handler_end());
    auto channel = GNUNET_CADET_channel_create(cadet, channel_ptr, &peer, &hash, nullptr, cadet_disconnect_client_trampoline, handlers.data());
    channel_ptr->channel = channel;
    channel_ptr->setConnectionOptions(options);
    return channel_ptr;
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
    if(!channel)
        throw std::runtime_error("CADET channel is not open");
    const size_t total_size = size + sizeof(struct GNUNET_MessageHeader);

    if(total_size > std::numeric_limits<uint16_t>::max())
        throw std::runtime_error("CADET message is too large");

    struct GNUNET_MessageHeader *msg = nullptr;
    auto env = GNUNET_MQ_msg_extra(msg, total_size, type);
    if(options) {
        // HACK: Check the incoming options value is valid in the API (GNUnet is very C..
        // so it is using enums as defines)
        constexpr size_t enum_size = sizeof(GNUNET_MQ_PriorityPreferences);
        if constexpr(enum_size == 1)
            GNUNET_assert(options <= std::numeric_limits<uint8_t>::max());
        else if constexpr(enum_size == 2)
            GNUNET_assert(options <= std::numeric_limits<uint16_t>::max());
        else if constexpr(enum_size == 4)
            GNUNET_assert(options <= std::numeric_limits<uint32_t>::max());
        else if constexpr(enum_size == 8)
            GNUNET_assert(options <= std::numeric_limits<uint64_t>::max());

        GNUNET_MQ_env_set_options(env, (GNUNET_MQ_PriorityPreferences)*options);
    }
    memcpy(&msg[1], data, size);
    GNUNET_MQ_send(getMQ(), env);
}

void CADETChannel::send(const std::string_view sv, uint16_t type)
{
    send(sv.data(), sv.size(), type);
}

void CADETChannel::disconnect()
{
    if(channel) {
        if(disconnectCallback)
            disconnectCallback();
        GNUNET_CADET_channel_destroy(channel);
        channel = nullptr;
    }
}

GNUNET_PeerIdentity CADETChannel::peer() const
{
    if(!channel)
        throw std::runtime_error("CADET channel is not open");
    auto info = GNUNET_CADET_channel_get_info(channel, GNUNET_CADET_OPTION_PEER);
    if(!info)
        throw std::runtime_error("Failed to get peer info");
    return info->peer;
}
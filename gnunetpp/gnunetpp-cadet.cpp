#include "gnunetpp-cadet.hpp"
#include "gnunetpp-crypto.hpp"
#include <gnunet/gnunet_protocols.h>
#include <gnunet/gnunet_util_lib.h>

#include <iostream>
#include <stdexcept>
#include <numeric>

using namespace gnunetpp;

struct PortListenerPack
{
    CADET* cadet;
    CADETChannelPtr channel;
};

struct ConnectPack
{
    std::weak_ptr<CADETChannel> channel;
};

namespace gnunetpp::internal
{
struct OpenPortCallbackPack
{
    GNUNET_HashCode port;
    CADET* self;
};
}


// Note this function returns a pointer as the closure of the connection
static void *cadet_connection_trampoline (
    void *cls,
    GNUNET_CADET_Channel *channel,
    const GNUNET_PeerIdentity *source)

{
    auto callback_pack = reinterpret_cast<internal::OpenPortCallbackPack*>(cls);
    auto cadet = callback_pack->self;
    const auto& port_hash = callback_pack->port;

    auto pack = new PortListenerPack();
    pack->cadet = cadet;
    pack->channel = std::make_shared<CADETChannel>(channel);
    pack->channel->setLocalPort(port_hash);
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
    else {
        if(cadet->disconnectedCallback)
            cadet->disconnectedCallback(portListenerPack->channel);
    }
    // HACK: GNUnet already destroyed the channel, so we don't want the destructor to try to destroy it again
    portListenerPack->channel->channel = nullptr;
    delete portListenerPack;
}

static void cadet_disconnect_client_trampoline(void *cls, const GNUNET_CADET_Channel *channel)
{
    auto pack = static_cast<ConnectPack*>(cls);
    auto channel_ptr = pack->channel.lock();
    GNUNET_assert(channel_ptr);
    if(channel_ptr->disconnectCallback)
        channel_ptr->disconnectCallback();
    delete pack;
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
    GNUNET_assert(message_end >= message_begin);
    GNUNET_assert(pack->channel->channel != nullptr);

    auto channel = pack->channel;
    std::string_view message(message_begin, message_end - message_begin);
    if(channel->readCallback)
        channel->readCallback(message, type);
    else if(pack->cadet->readCallback)
        pack->cadet->readCallback(channel, message, type);
    GNUNET_CADET_receive_done(channel->channel);
}

static void cadet_message_client_trampoline(void *cls, const struct GNUNET_MessageHeader *msg)
{
    auto pack = static_cast<ConnectPack*>(cls);
    auto channel_ptr = pack->channel.lock();
    GNUNET_assert(channel_ptr);

    auto size = ntohs(msg->size);
    auto type = ntohs(msg->type);
    auto message_begin = reinterpret_cast<const char*>(msg) + sizeof(GNUNET_MessageHeader);
    auto message_end = message_begin + size - sizeof(GNUNET_MessageHeader);
    if(channel_ptr->readCallback)
        channel_ptr->readCallback(std::string_view(message_begin, message_end - message_begin), type);
    
    GNUNET_assert(channel_ptr->channel != nullptr);
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
        for(auto [port, _] : open_ports)
            closePort(port);
        GNUNET_CADET_disconnect(cadet);
        cadet = nullptr;
    }
}

GNUNET_CADET_Port* CADET::openPort(const std::string_view port, const std::vector<uint16_t>& acceptable_reply_types)
{
    std::vector<GNUNET_MQ_MessageHandler> handlers;
    handlers.reserve(acceptable_reply_types.size() + 1);
    for(auto type : acceptable_reply_types) {
        handlers.push_back({
            accept_all,
            cadet_message_trampoline,
            this,
            type,
            0
        });
    }
    handlers.push_back(GNUNET_MQ_handler_end());
    auto hash = crypto::hash(port);
    auto pack = new internal::OpenPortCallbackPack{};
    pack->self = this;
    pack->port = hash;
    auto cadet_port = GNUNET_CADET_open_port(cadet, &hash, cadet_connection_trampoline, pack, nullptr, cadet_disconnect_trampoline, handlers.data());
    if(!cadet_port)
        throw std::runtime_error("Failed to open CADET port");
    GNUNET_assert(open_ports.find(cadet_port) == open_ports.end());
    open_ports.insert({cadet_port, pack});
    return cadet_port;
}

void CADET::closePort(GNUNET_CADET_Port* port)
{
    auto it = open_ports.find(port);
    GNUNET_assert(it != open_ports.end());
    delete it->second;
    open_ports.erase(it);
    GNUNET_CADET_close_port(port);
}

CADETChannelPtr CADET::connect(const GNUNET_PeerIdentity& peer, const std::string_view port
    , const std::vector<uint16_t>& acceptable_reply_types
    , std::optional<uint32_t> options)
{
    auto channel_ptr = std::make_shared<CADETChannel>();
    auto hash = crypto::hash(port);
    auto pack = new ConnectPack;
    pack->channel = channel_ptr;
    channel_ptr->setRemotePort(hash);
    std::vector<GNUNET_MQ_MessageHandler> handlers;
    handlers.reserve(acceptable_reply_types.size() + 1);
    for(auto type : acceptable_reply_types) {
        handlers.push_back({
            accept_all,
            cadet_message_client_trampoline,
            pack,
            type,
            0
        });
    }
    handlers.push_back(GNUNET_MQ_handler_end());
    auto channel = GNUNET_CADET_channel_create(cadet, pack, &peer, &hash, nullptr, cadet_disconnect_client_trampoline, handlers.data());
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

void CADET::listPeers(const GNUNET_CONFIGURATION_Handle* cfg, std::function<void(const std::vector<GNUNET_CADET_PeerListEntry>&)> callback)
{
    auto cls = new ListPeersCallbackPack{callback, {}};
    GNUNET_CADET_list_peers(cfg, list_peers_trampoline, cls);
}

cppcoro::task<std::vector<GNUNET_CADET_PeerListEntry>> CADET::listPeers(const GNUNET_CONFIGURATION_Handle* cfg)
{
    struct PeerListAwaiter : public EagerAwaiter<std::vector<GNUNET_CADET_PeerListEntry>> {
        PeerListAwaiter(const GNUNET_CONFIGURATION_Handle* cfg) {
            listPeers(cfg, [this](const std::vector<GNUNET_CADET_PeerListEntry>& peers) {
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

    if(size > std::numeric_limits<uint16_t>::max())
        throw std::runtime_error("CADET message is too large");

    struct GNUNET_MessageHeader *msg = nullptr;
    auto env = GNUNET_MQ_msg_extra(msg, size, type);
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

void CADETChannel::setLocalPort(const GNUNET_HashCode& port)
{
    local_port = port;
}

void CADETChannel::setRemotePort(const GNUNET_HashCode& port)
{
    remote_port = port;
}

bool CADETChannel::isIncoming() const
{
    return local_port != crypto::zeroHash();
}

bool CADETChannel::isOutgoing() const
{
    return remote_port != crypto::zeroHash();
}

struct ListPathsCallbackPack
{
    std::function<void(const std::vector<std::vector<GNUNET_PeerIdentity>>&)> callback;
    std::vector<std::vector<GNUNET_PeerIdentity>> paths;
};

void list_paths_trampoline(void* cls, const GNUNET_CADET_PeerPathDetail* ppd)
{
    auto pack = static_cast<ListPathsCallbackPack*>(cls);
    GNUNET_assert(pack != nullptr);
    if(ppd) {
        std::vector<GNUNET_PeerIdentity> path;
        path.reserve(ppd->path_length);
        for(size_t i = 0; i < ppd->path_length; i++) {
            path.push_back(ppd->path[i]);
            // GNUnet specifically says then the target peer is the last entry in the path
            if(memcmp(&ppd->peer, &ppd->path[i], sizeof(GNUNET_PeerIdentity)) == 0)
                break;
        }
        pack->paths.emplace_back(std::move(path));
        return;
    }

    pack->callback(pack->paths);
    delete pack;
}

void CADET::pathsToPeer(const GNUNET_CONFIGURATION_Handle* cfg, const GNUNET_PeerIdentity& peer, std::function<void(const std::vector<std::vector<GNUNET_PeerIdentity>>&)> callback)
{
    auto cls = new ListPathsCallbackPack{std::move(callback), {}};
    GNUNET_CADET_get_path(cfg, &peer, list_paths_trampoline, cls);
}

cppcoro::task<std::vector<std::vector<GNUNET_PeerIdentity>>> CADET::pathsToPeer(const GNUNET_CONFIGURATION_Handle* cfg, const GNUNET_PeerIdentity& peer)
{
    struct PathAwaiter : public EagerAwaiter<std::vector<std::vector<GNUNET_PeerIdentity>>> {
        PathAwaiter(const GNUNET_CONFIGURATION_Handle* cfg, const GNUNET_PeerIdentity& peer) {
            pathsToPeer(cfg, peer, [this](const std::vector<std::vector<GNUNET_PeerIdentity>>& paths) {
                setValue(paths);
            });
        }
    };
    co_return co_await PathAwaiter(cfg, peer);
}

struct ListTunnelsCallbackPack
{
    std::function<void(const std::vector<GNUNET_CADET_TunnelDetails>&)> callback;
    std::vector<GNUNET_CADET_TunnelDetails> tunnels;
};

void list_tunnels_trampoline(void* cls, const GNUNET_CADET_TunnelDetails* td)
{
    auto pack = static_cast<ListTunnelsCallbackPack*>(cls);
    GNUNET_assert(pack != nullptr);
    if(td) {
        pack->tunnels.push_back(*td);
        return;
    }

    pack->callback(pack->tunnels);
    delete pack;
}

void CADET::listTunnels(const GNUNET_CONFIGURATION_Handle* cfg, std::function<void(const std::vector<GNUNET_CADET_TunnelDetails>&)> callback)
{
    auto cls = new ListTunnelsCallbackPack{std::move(callback), {}};
    GNUNET_CADET_list_tunnels(cfg, list_tunnels_trampoline, cls);
}

cppcoro::task<std::vector<GNUNET_CADET_TunnelDetails>> CADET::listTunnels(const GNUNET_CONFIGURATION_Handle* cfg)
{
    struct TunnelAwaiter : public EagerAwaiter<std::vector<GNUNET_CADET_TunnelDetails>> {
        TunnelAwaiter(const GNUNET_CONFIGURATION_Handle* cfg) {
            listTunnels(cfg, [this](const std::vector<GNUNET_CADET_TunnelDetails>& tunnels) {
                setValue(tunnels);
            });
        }
    };
    co_return co_await TunnelAwaiter(cfg);
}
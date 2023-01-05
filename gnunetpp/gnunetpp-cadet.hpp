#pragma once

#include <gnunet/gnunet_cadet_service.h>

#include "inner/Infra.hpp"
#include "gnunetpp-crypto.hpp"

#include <map>
#include <memory>
#include <functional>
#include <string_view>
#include <any>

namespace gnunetpp
{
namespace internal
{
struct OpenPortCallbackPack;
}

struct CADETChannel : public NonCopyable
{
    CADETChannel(GNUNET_CADET_Channel* channel) : channel(channel) {
    }
    CADETChannel() = default;
    ~CADETChannel() {
        if(channel)
            GNUNET_CADET_channel_destroy(channel);
    } 
    CADETChannel(CADETChannel&& other) : channel(other.channel) { other.channel = nullptr; }
    CADETChannel& operator=(CADETChannel&& other) { channel = other.channel; other.channel = nullptr; return *this; }

    /**
     * @brief Send a message to the connected peer
     * 
     * @param data pointer to the data to send
     * @param size size of the data to send
     * @param type type of the message
     */
    void send(const void* data, size_t size, uint16_t type);
    void send(const std::string_view sv, uint16_t type);

    /**
     * @brief Set the callback to be called when the channel is disconnected
     * 
     * @param cb callback to be called
     */
    void setDisconnectCallback(std::function<void()> cb) { disconnectCallback = std::move(cb); }
    /**
     * @brief Set connection options that controls priority, reliability, etc.
     * 
     * @param options options to set
     */
    void setConnectionOptions(std::optional<uint32_t> options) { this->options = options; }
    /**
     * @brief Set the callback to be called when a message is received
     * 
     * @param cb callback to be called
     */
    void setReceiveCallback(std::function<void(const std::string_view, uint16_t)> cb) { readCallback = std::move(cb); }
    /**
     * @brief Disconnect the channel
     */
    void disconnect();
    /**
     * @brief Get the peer identity of the connected peer
     * 
     * @return the peer identity of the connected peer
     */
    GNUNET_PeerIdentity peer() const;

    /**
     * @brief Set context data
     */
    void setContext(std::any context) { this->context_ = std::move(context); }
    /**
     * @brief Get context data
     */
    template <typename T>
    T& context() { return std::any_cast<T&>(context_); }
    template <typename T>
    const T& context() const { return std::any_cast<const T&>(context_); }

    /**
     * @brief Set the Local and remote port information
     * 
     * @param port the hash of the port name
     */
    void setLocalPort(const GNUNET_HashCode& port);
    void setRemotePort(const GNUNET_HashCode& port);

    /**
     * @brief Get the local and remote port information
     * @note There's no ephemeral ports in CADET. Only either the local or the remote port is set.
     * 
     * @return const GNUNET_HashCode& 
     */
    const GNUNET_HashCode& getLocalPort() const { return local_port; }
    const GNUNET_HashCode& getRemotePort() const { return remote_port;}
    const GNUNET_HashCode& remotePort() const { return getRemotePort(); }
    const GNUNET_HashCode& localPort() const { return getLocalPort(); }

    /**
     * @brief Is the channel incoming or outgoing (i.e. was it created by us or by the peer)
     * @note There's no ephemeral ports in CADET. isIncoming() and isOutgoing() are mutually exclusive.
     */
    bool isIncoming() const;
    bool isOutgoing() const;

    /**
     * @brief Get the port that is used for this channel
     * @note In CADET ther's no ephemeral ports. There's only the port on the sending side.
     *      This function returns that port no matter if the channel is incoming or outgoing.
     * 
     * @return const GNUNET_HashCode& 
     */
    const GNUNET_HashCode& port() const { return isIncoming() ? localPort() : remotePort(); }

    GNUNET_MQ_Handle* getMQ() const { return GNUNET_CADET_get_mq(channel); }    
    std::function<void(const std::string_view, uint16_t)> readCallback;
    std::function<void()> disconnectCallback;
    GNUNET_CADET_Channel* channel = nullptr;
    std::optional<uint32_t> options = 0;
    GNUNET_HashCode local_port = crypto::zeroHash();
    GNUNET_HashCode remote_port = crypto::zeroHash();
    std::any context_;
};
using CADETChannelPtr = std::shared_ptr<CADETChannel>;

struct CADET : public Service
{
    // these are part of gnunet-service-cadet_service.h not part of the public API as of 0.19.0
    // The values are copied from the source code and should be kept in sync with the source code.
    enum class EncryptionState
    {
        Uninitialized,
        AXSent,
        AXReceived,
        AXSentAndReceived,
        AxAuxSent,
        Ok,
    };

    enum class ConnectionState
    {
        New,
        Search, // Not sure, plain guess
        Wait,
        Ready,
        Shutdown,
    };
    CADET(const GNUNET_CONFIGURATION_Handle* cfg);
    ~CADET();
    void shutdown() override;

    /**
     * @brief Open a port to listen for incoming connections
     * 
     * @param port Port to listen on
     * @param acceptable_reply_types Types of messages that can be sent to this port 
     * @return GNUNET_CADET_Port* handle that can be used to close the port
     */
    GNUNET_CADET_Port* openPort(const std::string_view port, const std::vector<uint16_t>& acceptable_reply_types);
    /**
     * @brief Close a port
     * 
     * @param port Handle of the port to close
     */
    void closePort(GNUNET_CADET_Port* port);

    /**
     * @brief Connect to a peer via CADET
     * 
     * @param peer Peer to connect to
     * @param port Port to connect to
     * @param acceptable_reply_types Message types that we can receive from the peer
     * @param options Connection options that controls priority, reliability, etc.
     * @return ChannelPtr A communication channel to the peer
     */
    CADETChannelPtr connect(const GNUNET_PeerIdentity& peer, const std::string_view port
        , const std::vector<uint16_t>& acceptable_reply_types
        , std::optional<uint32_t> options = std::nullopt);

    /**
     * @brief Get a list of peers that we know about
     * 
     * @param cfg Handle to GNUnet
     */
    static void listPeers(const GNUNET_CONFIGURATION_Handle* cfg, std::function<void(const std::vector<GNUNET_CADET_PeerListEntry>&)>);
    static cppcoro::task<std::vector<GNUNET_CADET_PeerListEntry>> listPeers(const GNUNET_CONFIGURATION_Handle* cfg);

    /**
     * @brief Get list of paths to a peer
     * 
     * @param cfg Handle to GNUnet
     * @param peer Peer to get paths to
     */
    static void pathsToPeer(const GNUNET_CONFIGURATION_Handle* cfg, const GNUNET_PeerIdentity& peer, std::function<void(const std::vector<std::vector<GNUNET_PeerIdentity>>&)> callback);
    static cppcoro::task<std::vector<std::vector<GNUNET_PeerIdentity>>> pathsToPeer(const GNUNET_CONFIGURATION_Handle* cfg, const GNUNET_PeerIdentity& peer);

    static void listTunnels(const GNUNET_CONFIGURATION_Handle* cfg, std::function<void(const std::vector<GNUNET_CADET_TunnelDetails>&)> callback);
    static cppcoro::task<std::vector<GNUNET_CADET_TunnelDetails>> listTunnels(const GNUNET_CONFIGURATION_Handle* cfg);

    /**
     * @brief Set the callback to be called when a connection is established **to** this peer
     * 
     * @param cb callback to be called
     */
    void setConnectedCallback(std::function<void(const CADETChannelPtr&)> cb) { connectedCallback = std::move(cb); }
    /**
     * @brief Set the callback to be called when a connection is disconnected and that connection was **inbound** to this peer
     * 
     * @param cb callback to be called
     */
    void setDisconnectedCallback(std::function<void(const CADETChannelPtr&)> cb) { disconnectedCallback = std::move(cb); }

    /**
     * @brief Set the callback to be called when a message is received
     * 
     * @param cb callback to be called
     */
    void setReceiveCallback(std::function<void(const CADETChannelPtr&, const std::string_view, uint16_t)> cb) { readCallback = std::move(cb); }

    GNUNET_CADET_Handle* nativeHandle() const { return cadet; }

    std::map<GNUNET_CADET_Port*, internal::OpenPortCallbackPack*> open_ports;
    GNUNET_CADET_Handle* cadet = nullptr;
    std::function<void(const CADETChannelPtr&)> connectedCallback;
    std::function<void(const CADETChannelPtr&, const std::string_view, uint16_t)> readCallback; 
    std::function<void(const CADETChannelPtr&)> disconnectedCallback;
};
}

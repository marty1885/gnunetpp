#pragma once

#include <gnunet/gnunet_cadet_service.h>

#include "inner/Infra.hpp"

#include <set>
#include <memory>
#include <functional>
#include <string_view>

namespace gnunetpp
{

struct CADETChannel
{
    CADETChannel(GNUNET_CADET_Channel* channel) : channel(channel) {}
    CADETChannel() = default;
    ~CADETChannel() {
        if(channel)
            GNUNET_CADET_channel_destroy(channel);
    } 
    CADETChannel(const CADETChannel&) = delete;
    CADETChannel& operator=(const CADETChannel&) = delete;
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


    GNUNET_MQ_Handle* getMQ() const { return GNUNET_CADET_get_mq(channel); }    
    std::function<void(const std::string_view, uint16_t)> readCallback;
    std::function<void()> disconnectCallback;
    GNUNET_CADET_Channel* channel = nullptr;
    std::optional<uint32_t> options = 0;
};

struct CADET : public Service
{
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
     * @return CADETChannel* A communication channel to the peer
     */
    CADETChannel* connect(const GNUNET_PeerIdentity& peer, const std::string_view port
        , const std::vector<uint16_t>& acceptable_reply_types
        , std::optional<uint32_t> options = std::nullopt);

    /**
     * @brief Get a list of peers that we know about
     * 
     * @param cfg Handle to GNUnet
     */
    static void list_peers(const GNUNET_CONFIGURATION_Handle* cfg, std::function<void(const std::vector<GNUNET_CADET_PeerListEntry>&)>);
    static cppcoro::task<std::vector<GNUNET_CADET_PeerListEntry>> list_peers(const GNUNET_CONFIGURATION_Handle* cfg);

    /**
     * @brief Get list of paths to a peer
     * 
     * @param cfg Handle to GNUnet
     * @param peer Peer to get paths to
     */
    static void get_path(const GNUNET_CONFIGURATION_Handle* cfg, const GNUNET_PeerIdentity& peer, std::function<void(const std::vector<std::vector<GNUNET_PeerIdentity>>&)> callback);
    static cppcoro::task<std::vector<std::vector<GNUNET_PeerIdentity>>> get_path(const GNUNET_CONFIGURATION_Handle* cfg, const GNUNET_PeerIdentity& peer);

    /**
     * @brief Set the callback to be called when a connection is established **to** this peer
     * 
     * @param cb callback to be called
     */
    void setConnectedCallback(std::function<void(CADETChannel*)> cb) { connectedCallback = std::move(cb); }
    /**
     * @brief Set the callback to be called when a connection is disconnected and that connection was **inbound** to this peer
     * 
     * @param cb callback to be called
     */
    void setDisconnectedCallback(std::function<void(CADETChannel*)> cb) { disconectedCallback = std::move(cb); }

    GNUNET_CADET_Handle* nativeHandle() const { return cadet; }

    std::set<GNUNET_CADET_Port*> open_ports;
    GNUNET_CADET_Handle* cadet = nullptr;
    std::function<void(CADETChannel*)> connectedCallback;
    std::function<void(CADETChannel*)> disconectedCallback;
};
}

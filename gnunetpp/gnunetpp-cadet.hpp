#pragma once

#include <gnunet/gnunet_cadet_service.h>

#include "inner/Infra.hpp"

#include <map>
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

    void send(const void* data, size_t size, uint16_t type);
    void send(const std::string_view sv, uint16_t type);
    void setDisconnectCallback(std::function<void()> cb) { disconnectCallback = std::move(cb); }
    void setConnectionOptions(std::optional<uint32_t> options) { this->options = options; }
    void setReceiveCallback(std::function<void(const std::string_view, uint16_t)> cb) { readCallback = std::move(cb); }

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

    GNUNET_CADET_Port* openPort(const std::string_view port);
    void closePort(GNUNET_CADET_Port* port);

    CADETChannel* connect(const GNUNET_PeerIdentity& peer, const std::string_view port, std::optional<uint32_t> options = std::nullopt);

    static void list_peers(const GNUNET_CONFIGURATION_Handle* cfg, std::function<void(const std::vector<GNUNET_CADET_PeerListEntry>&)>);
    static cppcoro::task<std::vector<GNUNET_CADET_PeerListEntry>> list_peers(const GNUNET_CONFIGURATION_Handle* cfg);

    GNUNET_CADET_Handle* nativeHandle() const { return cadet; }

    GNUNET_CADET_Handle* cadet = nullptr;
    std::map<GNUNET_CADET_Channel*, std::unique_ptr<CADETChannel>> openChannels;
};
}

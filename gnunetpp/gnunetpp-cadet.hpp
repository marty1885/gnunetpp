#pragma once

#include <gnunet/gnunet_cadet_service.h>

#include "inner/Infra.hpp"

#include <map>
#include <memory>
#include <functional>

namespace gnunetpp
{

struct CADETChannel
{
    CADETChannel(GNUNET_CADET_Channel* channel) : channel(channel) {}
    ~CADETChannel() { GNUNET_CADET_channel_destroy(channel); }
    GNUNET_CADET_Channel* channel;
    
    std::function<void(const std::string_view)> readCallback;
    std::function<void()> disconnectCallback;
};

struct CADET : public Service
{
    CADET(const GNUNET_CONFIGURATION_Handle* cfg);
    ~CADET();
    void shutdown() override;

    GNUNET_CADET_Port* openPort(const std::string_view port);
    void closePort(GNUNET_CADET_Port* port);

    void connect(const std::string_view port, const GNUNET_PeerIdentity& peer);

    static void list_peers(const GNUNET_CONFIGURATION_Handle* cfg, std::function<void(const std::vector<GNUNET_CADET_PeerListEntry>&)>);
    static cppcoro::task<std::vector<GNUNET_CADET_PeerListEntry>> list_peers(const GNUNET_CONFIGURATION_Handle* cfg);

    GNUNET_CADET_Handle* cadet = nullptr;
    std::map<GNUNET_CADET_Channel*, std::unique_ptr<CADETChannel>> openChannels;
};
}
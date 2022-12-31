#pragma once

#include <gnunet/gnunet_util_lib.h>
#include <gnunet/gnunet_peerinfo_service.h>

#include "inner/Infra.hpp"

namespace gnunetpp
{

struct PeerInfo : public Service
{
    PeerInfo(const GNUNET_CONFIGURATION_Handle* cfg);
    void shutdown() override;
    ~PeerInfo();

    void getPeerInfo(const GNUNET_PeerIdentity& peer, std::function<void(const GNUNET_HELLO_Address* addr)> callback, std::function<void(const std::string_view)> errorCallback);

    GNUNET_PEERINFO_Handle* native_handle() const { return handle;}
    GNUNET_PEERINFO_Handle* handle = nullptr;
};

}
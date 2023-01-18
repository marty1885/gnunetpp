#pragma once

#include <gnunet/gnunet_util_lib.h>
#include <gnunet/gnunet_peerinfo_service.h>

#include "gnunetpp-crypto.hpp"
#include "inner/Infra.hpp"
#include "inner/coroutine.hpp"
#include <set>

namespace gnunetpp
{

struct PeerInfo : public Service
{
    PeerInfo(const GNUNET_CONFIGURATION_Handle* cfg);
    void shutdown() override;
    ~PeerInfo();

    void peers(std::function<void(const std::set<GNUNET_PeerIdentity> peers)> callback, std::function<void(const std::string_view)> errorCallback);
    cppcoro::task<std::set<GNUNET_PeerIdentity>> peers();

    GNUNET_PEERINFO_Handle* native_handle() const { return handle;}
    GNUNET_PEERINFO_Handle* handle = nullptr;
};

}
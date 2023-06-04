#pragma once

#include <gnunet/gnunet_util_lib.h>
#include <gnunet/gnunet_peerinfo_service.h>
#include <gnunet/gnunet_transport_hello_service.h>

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
    Task<std::set<GNUNET_PeerIdentity>> peers();

    void addPeer(const std::shared_ptr<GNUNET_HELLO_Message>& hello, std::function<void()> callback);
    Task<void> addPeer(std::shared_ptr<GNUNET_HELLO_Message> hello);

    GNUNET_PEERINFO_Handle* native_handle() const { return handle;}
    GNUNET_PEERINFO_Handle* handle = nullptr;
};

void helloMessage(const GNUNET_CONFIGURATION_Handle* cfg, GNUNET_TRANSPORT_AddressClass ac, std::function<void(std::shared_ptr<GNUNET_HELLO_Message> hello)> callback);
Task<std::shared_ptr<GNUNET_HELLO_Message>> helloMessage(const GNUNET_CONFIGURATION_Handle* cfg, GNUNET_TRANSPORT_AddressClass ac=GNUNET_TRANSPORT_AC_ANY);
std::string to_string(const std::shared_ptr<GNUNET_HELLO_Message> hello);
}
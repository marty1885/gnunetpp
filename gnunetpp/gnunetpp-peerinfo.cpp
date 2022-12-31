#include "gnunetpp-peerinfo.hpp"

using namespace gnunetpp;

PeerInfo::PeerInfo(const GNUNET_CONFIGURATION_Handle* cfg)
{
    handle = GNUNET_PEERINFO_connect(cfg);
    registerService(this);
}

void PeerInfo::shutdown()
{
    if(handle) {
        GNUNET_PEERINFO_disconnect(handle);
        handle = nullptr;
    }
}

PeerInfo::~PeerInfo()
{
    shutdown();
    removeService(this);
}

struct PeerInfoCallbackPack
{
    std::function<void(const GNUNET_HELLO_Address* addr)> callback;
    std::function<void(const std::string_view)> errorCallback;
};

void PeerInfo::getPeerInfo(const GNUNET_PeerIdentity& peer, std::function<void(const GNUNET_HELLO_Address* addr)> callback, std::function<void(const std::string_view)> errorCallback)
{
    auto pack = new PeerInfoCallbackPack{std::move(callback), std::move(errorCallback)};
    auto ic = GNUNET_PEERINFO_iterate(handle, GNUNET_NO, &peer
        , [](void *cls,
            const struct GNUNET_PeerIdentity *peer,
            const struct GNUNET_HELLO_Message *hello,
            const char *err_msg) {
        auto pack = reinterpret_cast<PeerInfoCallbackPack*>(cls);
        if(err_msg) {
            pack->errorCallback("Error in peer info");
            delete pack;
        }
        else if(peer == nullptr) {
            delete pack;
        }
        else {
            // auto addr = GNUNET_HELLO_iterate_addresses(hello, nullptr);
            // if(addr) {
            //     pack->callback(addr);
            // }
        }
        delete pack;
    }, pack);
    if(!ic) {
        errorCallback("Failed to iterate peer info");
    }
}
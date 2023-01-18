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
    std::function<void(const std::set<GNUNET_PeerIdentity> addr)> callback;
    std::function<void(const std::string_view)> errorCallback;
    std::set<GNUNET_PeerIdentity> peers;
};

void PeerInfo::peers(std::function<void(const std::set<GNUNET_PeerIdentity> peer_ids)> callback, std::function<void(const std::string_view)> errorCallback)
{
    auto pack = new PeerInfoCallbackPack{std::move(callback), std::move(errorCallback)};
    auto ic = GNUNET_PEERINFO_iterate(handle, GNUNET_NO, nullptr
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
            pack->callback(std::move(pack->peers));
            delete pack;
        }
        else {
            pack->peers.insert(*peer);
        }
    }, pack);
    if(!ic) {
        errorCallback("Failed to iterate peer info");
    }
}

cppcoro::task<std::set<GNUNET_PeerIdentity>> PeerInfo::peers()
{
    struct PeerInfoAwaiter : public EagerAwaiter<std::set<GNUNET_PeerIdentity>>
    {
        PeerInfoAwaiter(PeerInfo& pi)
            : pi(pi)
        {
            pi.peers([this](const std::set<GNUNET_PeerIdentity> peers) {
                setValue(peers);
            }, [this](const std::string_view err) {
                setException(std::make_exception_ptr(std::runtime_error(err.data())));
            });
        }

        PeerInfo& pi;
    };
    co_return co_await PeerInfoAwaiter(*this);
}
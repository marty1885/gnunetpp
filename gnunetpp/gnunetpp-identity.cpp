#include "gnunetpp-identity.hpp"
#include "gnunetpp-scheduler.hpp"

#include <stdexcept>
#include <iostream>

struct IdentityCallbackPack
{
    std::function<void(const std::string&, GNUNET_IDENTITY_Ego*)> fn;
    GNUNET_IDENTITY_Handle* handle;
};

namespace gnunetpp
{
namespace detail
{
static void identity_create_trampline(void* cls, const struct GNUNET_IDENTITY_PrivateKey* id, GNUNET_ErrorCode ec)
{
    auto cb = reinterpret_cast<std::function<void(const GNUNET_IDENTITY_PrivateKey* sk, const std::string& err)>*>(cls);
    std::string err_msg = ec != GNUNET_EC_NONE ? GNUNET_ErrorCode_get_hint(ec) : "";
    (*cb)(id, err_msg);
    delete cb;
}

static void identity_delete_trampline(void* cls, GNUNET_ErrorCode ec)
{
    auto cb = reinterpret_cast<std::function<void(const std::string& err)>*>(cls);
    std::string err_msg = ec != GNUNET_EC_NONE ? GNUNET_ErrorCode_get_hint(ec) : "";
    (*cb)(err_msg);
    delete cb;
}

static void ego_lookup_trampline(void* cls, struct GNUNET_IDENTITY_Ego* ego)
{
    auto cb = reinterpret_cast<std::function<void(std::optional<Ego>)>*>(cls);
    std::optional<Ego> opt_ego;
    if(ego != nullptr)
        opt_ego = Ego(ego);
    (*cb)(std::move(opt_ego));
    delete cb;
}

static void identity_info_trampoline(void *cls,
           struct GNUNET_IDENTITY_Ego *ego,
           void **ctx,
           const char *identifier)
{
    auto pack = reinterpret_cast<IdentityCallbackPack*>(cls);
    if(ego == nullptr) {
        // iteration finished
        gnunetpp::scheduler::run([pack](){
            GNUNET_IDENTITY_disconnect(pack->handle);
            delete pack;
        });
        pack->fn("", nullptr);
        return;
    }

    if(identifier == nullptr) // Deleted ego
        return;
    pack->fn(identifier, ego);
}

}

Ego::Ego(GNUNET_IDENTITY_Ego* ego)
    : ego(ego)
{
}

GNUNET_IDENTITY_PublicKey Ego::publicKey() const
{
    return getPublicKey(ego);
}

const GNUNET_IDENTITY_PrivateKey*  Ego::privateKey() const
{
    return getPrivateKey(ego);
}


IdentityService::IdentityService(const GNUNET_CONFIGURATION_Handle* cfg)
{
    handle = GNUNET_IDENTITY_connect(cfg, nullptr, nullptr);
    if(handle == nullptr)
	    throw std::runtime_error("GNUNET_IDENTITY_connect failed");
    registerService(this);
}

IdentityService::~IdentityService()
{
    shutdown();
    removeService(this);
}

void IdentityService::shutdown()
{
    if(handle != nullptr) {
        scheduler::run([handle=this->handle](){
            GNUNET_IDENTITY_disconnect(handle);
        });
        handle = nullptr;
    }
}

GNUNET_IDENTITY_Operation* IdentityService::createIdentity(const std::string& name
        , std::function<void(const GNUNET_IDENTITY_PrivateKey*, const std::string&)> fn
        , GNUNET_IDENTITY_KeyType type)
{
    auto cb = new std::function<void(const GNUNET_IDENTITY_PrivateKey*, const std::string&)>(fn);
    return GNUNET_IDENTITY_create(handle, name.c_str(), nullptr, type
        , &detail::identity_create_trampline, cb);
}

cppcoro::task<const GNUNET_IDENTITY_PrivateKey*> IdentityService::createIdentity(const std::string& name
        , GNUNET_IDENTITY_KeyType type)
{
    struct IdentityCreateAwaiter : public EagerAwaiter<const GNUNET_IDENTITY_PrivateKey*>
    {
        IdentityCreateAwaiter(IdentityService* identity, const std::string& name, GNUNET_IDENTITY_KeyType type)
        {
            identity->createIdentity(name, [this](const GNUNET_IDENTITY_PrivateKey* sk, const std::string& err){
                if(err.empty()) {
                    setValue(sk);
                } else {
                    setException(std::make_exception_ptr(std::runtime_error(err)));
                }
            }, type);
        }
    };
    co_return co_await IdentityCreateAwaiter(this, name, type);
}

GNUNET_IDENTITY_Operation* IdentityService::deleteIdentity(const std::string& name
        , std::function<void(const std::string&)> fn)
{
    auto cb = new std::function<void(const std::string&)>(fn);
    return GNUNET_IDENTITY_delete(handle, name.c_str(), &detail::identity_delete_trampline, cb);
}

cppcoro::task<> IdentityService::deleteIdentity(const std::string& name)
{
    struct IdentityDeleteAwaiter : public EagerAwaiter<void>
    {
        IdentityDeleteAwaiter(IdentityService* identity, const std::string& name)
        {
            identity->deleteIdentity(name, [this](const std::string& err){
                if(err.empty()) {
                    setValue();
                } else {
                    setException(std::make_exception_ptr(std::runtime_error(err)));
                }
            });
        }
    };
    co_return co_await IdentityDeleteAwaiter(this, name);
}

GNUNET_IDENTITY_EgoLookup* getEgo(const GNUNET_CONFIGURATION_Handle* cfg
    , const std::string& name, std::function<void(std::optional<Ego>)> fn)
{
    return GNUNET_IDENTITY_ego_lookup(cfg, name.c_str(), &detail::ego_lookup_trampline
        , new std::function<void(std::optional<Ego>)>(std::move(fn)));
}

cppcoro::task<std::optional<Ego>> getEgo(const GNUNET_CONFIGURATION_Handle* cfg
    , const std::string& name)
{
    struct EgoLookupAwaiter : public EagerAwaiter<std::optional<Ego>>
    {
        EgoLookupAwaiter(const GNUNET_CONFIGURATION_Handle* cfg, const std::string& name)
        {
            auto handle = getEgo(cfg, name, [this](std::optional<Ego> ego){
                setValue(ego);
            });
            if(handle == nullptr) {
                setException(std::make_exception_ptr(std::runtime_error("GNUNET_IDENTITY_ego_lookup failed")));
            }
        }
    };
    co_return co_await EgoLookupAwaiter(cfg, name);
}

void getIdentities(const GNUNET_CONFIGURATION_Handle* cfg, std::function<void(const std::string&, GNUNET_IDENTITY_Ego* ego)> fn)
{
    auto pack = new IdentityCallbackPack{std::move(fn), nullptr};
    auto handle = GNUNET_IDENTITY_connect(cfg, detail::identity_info_trampoline, pack);
    pack->handle = handle;
    if(handle == nullptr) {
        delete pack;
        throw std::runtime_error("GNUNET_IDENTITY_connect failed");
    }
}

Ego anonymousEgo()
{
    return Ego(GNUNET_IDENTITY_ego_get_anonymous());
}

GNUNET_IDENTITY_PublicKey getPublicKey(GNUNET_IDENTITY_Ego* ego)
{
    GNUNET_IDENTITY_PublicKey pk;
    GNUNET_IDENTITY_ego_get_public_key(ego, &pk);
    return pk;
}

const GNUNET_IDENTITY_PrivateKey* getPrivateKey(const GNUNET_IDENTITY_Ego* ego)
{
    return GNUNET_IDENTITY_ego_get_private_key(ego);
}

GNUNET_IDENTITY_KeyType Ego::keyType() const
{
    return getKeyType(ego);
}

std::string to_string(const GNUNET_IDENTITY_PrivateKey& key)
{
    char* str = GNUNET_IDENTITY_private_key_to_string(&key);
    std::string ret = str;
    GNUNET_free(str);
    return ret;
}

std::string to_string(const GNUNET_IDENTITY_PublicKey& key)
{
    char* str = GNUNET_IDENTITY_public_key_to_string(&key);
    std::string ret = str;
    GNUNET_free(str);
    return ret;
}

std::string to_string(GNUNET_IDENTITY_KeyType type)
{
    switch(type) {
        case GNUNET_IDENTITY_TYPE_ECDSA:
            return "ECDSA";
        case GNUNET_IDENTITY_TYPE_EDDSA:
            return "EdDSA";
        default:
            return "Unknown";
    }
}

GNUNET_IDENTITY_PublicKey getPublicKey(const GNUNET_IDENTITY_PrivateKey& key)
{
    GNUNET_IDENTITY_PublicKey pk;
    GNUNET_IDENTITY_key_get_public(&key, &pk);
    return pk;
}

GNUNET_IDENTITY_KeyType getKeyType(GNUNET_IDENTITY_Ego* ego)
{
    GNUNET_IDENTITY_PublicKey pk = getPublicKey(ego);
    return (GNUNET_IDENTITY_KeyType)ntohl(pk.type);
}

}
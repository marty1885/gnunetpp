#include "gnunetpp-identity.hpp"
#include "gnunetpp-scheduler.hpp"

#include <stdexcept>
#include <iostream>

struct IdentityCallbackPack
{
    std::function<void(const std::string&, GNUNET_IDENTITY_Ego*)> fn;
    GNUNET_IDENTITY_Handle* handle;
};

namespace gnunetpp::identity
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
    auto cb = reinterpret_cast<std::function<void(GNUNET_IDENTITY_Ego*)>*>(cls);
    (*cb)(const_cast<GNUNET_IDENTITY_Ego*>(ego));
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

GNUNET_IDENTITY_Operation* IdentityService::create_identity(const std::string& name
        , std::function<void(const GNUNET_IDENTITY_PrivateKey*, const std::string&)> fn
        , GNUNET_IDENTITY_KeyType type)
{
    auto cb = new std::function<void(const GNUNET_IDENTITY_PrivateKey*, const std::string&)>(fn);
    return GNUNET_IDENTITY_create(handle, name.c_str(), nullptr, type
        , &detail::identity_create_trampline, cb);
}

cppcoro::task<const GNUNET_IDENTITY_PrivateKey*> IdentityService::create_identity(const std::string& name
        , GNUNET_IDENTITY_KeyType type)
{
    struct IdentityCreateAwaiter : public EagerAwaiter<const GNUNET_IDENTITY_PrivateKey*>
    {
        IdentityCreateAwaiter(IdentityService* identity, const std::string& name, GNUNET_IDENTITY_KeyType type)
        {
            identity->create_identity(name, [this](const GNUNET_IDENTITY_PrivateKey* sk, const std::string& err){
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

GNUNET_IDENTITY_Operation* IdentityService::delete_identity(const std::string& name
        , std::function<void(const std::string&)> fn)
{
    auto cb = new std::function<void(const std::string&)>(fn);
    return GNUNET_IDENTITY_delete(handle, name.c_str(), &detail::identity_delete_trampline, cb);
}

cppcoro::task<> IdentityService::delete_identity(const std::string& name)
{
    struct IdentityDeleteAwaiter : public EagerAwaiter<void>
    {
        IdentityDeleteAwaiter(IdentityService* identity, const std::string& name)
        {
            identity->delete_identity(name, [this](const std::string& err){
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

GNUNET_IDENTITY_EgoLookup* lookup_ego(const GNUNET_CONFIGURATION_Handle* cfg
    , const std::string& name, std::function<void(GNUNET_IDENTITY_Ego*)> fn)
{
    return GNUNET_IDENTITY_ego_lookup(cfg, name.c_str(), &detail::ego_lookup_trampline
        , new std::function<void(GNUNET_IDENTITY_Ego*)>(std::move(fn)));
}

cppcoro::task<GNUNET_IDENTITY_Ego*> lookup_ego(const GNUNET_CONFIGURATION_Handle* cfg
    , const std::string& name)
{
    struct EgoLookupAwaiter : public EagerAwaiter<GNUNET_IDENTITY_Ego*>
    {
        EgoLookupAwaiter(const GNUNET_CONFIGURATION_Handle* cfg, const std::string& name)
        {
            auto handle = lookup_ego(cfg, name, [this](GNUNET_IDENTITY_Ego* ego){
                setValue(ego);
            });
            if(handle == nullptr) {
                setException(std::make_exception_ptr(std::runtime_error("GNUNET_IDENTITY_ego_lookup failed")));
            }
        }
    };
    co_return co_await EgoLookupAwaiter(cfg, name);
}

void get_identities(const GNUNET_CONFIGURATION_Handle* cfg, std::function<void(const std::string&, GNUNET_IDENTITY_Ego* ego)> fn)
{
    auto pack = new IdentityCallbackPack{std::move(fn), nullptr};
    auto handle = GNUNET_IDENTITY_connect(cfg, detail::identity_info_trampoline, pack);
    pack->handle = handle;
    if(handle == nullptr) {
        delete pack;
        throw std::runtime_error("GNUNET_IDENTITY_connect failed");
    }
}

GNUNET_IDENTITY_Ego* anonymous_ego()
{
    return GNUNET_IDENTITY_ego_get_anonymous();
}

GNUNET_IDENTITY_PublicKey get_public_key(GNUNET_IDENTITY_Ego* ego)
{
    GNUNET_IDENTITY_PublicKey pk;
    GNUNET_IDENTITY_ego_get_public_key(ego, &pk);
    return pk;
}

const GNUNET_IDENTITY_PrivateKey* get_private_key(const GNUNET_IDENTITY_Ego* ego)
{
    return GNUNET_IDENTITY_ego_get_private_key(ego);
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

GNUNET_IDENTITY_PublicKey get_public_key(const GNUNET_IDENTITY_PrivateKey& key)
{
    GNUNET_IDENTITY_PublicKey pk;
    GNUNET_IDENTITY_key_get_public(&key, &pk);
    return pk;
}

GNUNET_IDENTITY_KeyType get_key_type(GNUNET_IDENTITY_Ego* ego)
{
    GNUNET_IDENTITY_PublicKey pk = get_public_key(ego);
    return (GNUNET_IDENTITY_KeyType)ntohl(pk.type);
}

}
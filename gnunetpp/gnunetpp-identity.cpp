#include "gnunetpp-identity.hpp"

#include <stdexcept>
#include <iostream>

namespace gnunetpp::identity
{
namespace detail
{
static void identity_create_trampline(void* cls, const struct GNUNET_IDENTITY_PrivateKey* id, GNUNET_ErrorCode ec)
{
    auto cb = reinterpret_cast<std::function<void(const GNUNET_IDENTITY_PrivateKey& sk, const std::string& err)>*>(cls);
    std::string err_msg = GNUNET_ErrorCode_get_hint(ec);
    (*cb)(*id, err_msg);
    delete cb;
}

static void identity_delete_trampline(void* cls, GNUNET_ErrorCode ec)
{
    auto cb = reinterpret_cast<std::function<void(const std::string& err)>*>(cls);
    std::string err_msg = GNUNET_ErrorCode_get_hint(ec);
    (*cb)(err_msg);
    delete cb;
}

static void ego_lookup_trampline(void* cls, struct GNUNET_IDENTITY_Ego* ego)
{
    auto cb = reinterpret_cast<std::function<void(GNUNET_IDENTITY_Ego*)>*>(cls);
    (*cb)(const_cast<GNUNET_IDENTITY_Ego*>(ego));
    delete cb;
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
        GNUNET_IDENTITY_disconnect(handle);
        handle = nullptr;
    }
}

GNUNET_IDENTITY_Operation* IdentityService::create_identity(const std::string& name
        , std::function<void(const GNUNET_IDENTITY_PrivateKey&, const std::string&)> fn
        , GNUNET_IDENTITY_KeyType type)
{
    auto cb = new std::function<void(const GNUNET_IDENTITY_PrivateKey&, const std::string&)>(fn);
    return GNUNET_IDENTITY_create(handle, name.c_str(), nullptr, type
        , &detail::identity_create_trampline, cb);
}

GNUNET_IDENTITY_Operation* IdentityService::delete_identity(const std::string& name
        , std::function<void(const std::string&)> fn)
{
    auto cb = new std::function<void(const std::string&)>(fn);
    return GNUNET_IDENTITY_delete(handle, name.c_str(), &detail::identity_delete_trampline, cb);
}

GNUNET_IDENTITY_EgoLookup* lookup_ego(const GNUNET_CONFIGURATION_Handle* cfg
    , const std::string& name, std::function<void(GNUNET_IDENTITY_Ego*)> fn)
{
    return GNUNET_IDENTITY_ego_lookup(cfg, name.c_str(), &detail::ego_lookup_trampline
        , new std::function<void(GNUNET_IDENTITY_Ego*)>(std::move(fn)));
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
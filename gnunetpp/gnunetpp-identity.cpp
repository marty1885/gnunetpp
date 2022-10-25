#include "gnunetpp-identity.hpp"

#include <stdexcept>

namespace gnunetpp::identity
{
namespace detail
{
void identity_create_trampline(void* cls, const struct GNUNET_IDENTITY_PrivateKey* id, const char* emsg)
{
    auto cb = reinterpret_cast<std::function<void(const GNUNET_IDENTITY_PrivateKey& sk, const std::string& err)>*>(cls);
    std::string err_msg = emsg ? emsg : "";
    (*cb)(*id, err_msg);
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
}

void IdentityService::shutdown()
{
    if(handle != nullptr) {
        GNUNET_IDENTITY_disconnect(handle);
        handle = nullptr;
    }
    removeService(this);
}

GNUNET_IDENTITY_Operation* IdentityService::create_identity(const std::string& name
        , std::function<void(const GNUNET_IDENTITY_PrivateKey&, const std::string&)> fn)
{
    auto cb = new std::function<void(const GNUNET_IDENTITY_PrivateKey&, const std::string&)>(fn);
    return GNUNET_IDENTITY_create(handle, name.c_str(), nullptr, GNUNET_IDENTITY_TYPE_ECDSA
        , &detail::identity_create_trampline, cb);
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

GNUNET_IDENTITY_PublicKey get_public_key(const GNUNET_IDENTITY_PrivateKey& key)
{
    GNUNET_IDENTITY_PublicKey pk;
    GNUNET_IDENTITY_key_get_public(&key, &pk);
    return pk;
}


}
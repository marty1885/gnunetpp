#pragma once
#include <gnunet/platform.h>
#include <gnunet/gnunet_identity_service.h>
#include <gnunet/gnunet_crypto_lib.h>

#include "inner/Infra.hpp"

#include <functional>

struct GNUNET_CONFIGURATION_Handle;

namespace gnunetpp::identity
{

struct IdentityService : public Service
{
    IdentityService(const GNUNET_CONFIGURATION_Handle* cfg);
    ~IdentityService();

    void shutdown() override;

    GNUNET_IDENTITY_Operation* create_identity(const std::string& name
        , std::function<void(const GNUNET_IDENTITY_PrivateKey&, const std::string&)> fn
        , GNUNET_IDENTITY_KeyType type = GNUNET_IDENTITY_TYPE_ECDSA);
    GNUNET_IDENTITY_Operation* delete_identity(const std::string& name
        , std::function<void(const std::string&)> fn);

    GNUNET_IDENTITY_Handle* native_handle() const { return handle; }
    GNUNET_IDENTITY_Handle* handle = nullptr;
};

std::string to_string(const GNUNET_IDENTITY_PrivateKey& key);
std::string to_string(const GNUNET_IDENTITY_PublicKey& key);
std::string to_string(GNUNET_IDENTITY_KeyType key);

GNUNET_IDENTITY_PublicKey get_public_key(const GNUNET_IDENTITY_PrivateKey& key);
GNUNET_IDENTITY_EgoLookup* lookup_ego(const GNUNET_CONFIGURATION_Handle* cfg
    , const std::string& name, std::function<void(GNUNET_IDENTITY_Ego*)> fn);
GNUNET_IDENTITY_PublicKey get_public_key(GNUNET_IDENTITY_Ego* ego);
const GNUNET_IDENTITY_PrivateKey* get_private_key(const GNUNET_IDENTITY_Ego* ego);
GNUNET_IDENTITY_Ego* anonymous_ego();
GNUNET_IDENTITY_KeyType get_key_type(GNUNET_IDENTITY_Ego* ego);

}
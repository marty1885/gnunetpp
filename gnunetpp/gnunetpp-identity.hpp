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
        , std::function<void(const GNUNET_IDENTITY_PrivateKey&, const std::string&)> fn);
    GNUNET_IDENTITY_Operation* delete_identity(const std::string& name
        , std::function<void(const std::string&)> fn);

    GNUNET_IDENTITY_Handle* native_handle() const { return handle; }
    GNUNET_IDENTITY_Handle* handle;
};

std::string to_string(const GNUNET_IDENTITY_PrivateKey& key);
std::string to_string(const GNUNET_IDENTITY_PublicKey& key);

GNUNET_IDENTITY_PublicKey get_public_key(const GNUNET_IDENTITY_PrivateKey& key);
}
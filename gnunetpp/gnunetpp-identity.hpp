#pragma once
#include <gnunet/gnunet_identity_service.h>
#include <gnunet/gnunet_util_lib.h>

#include "inner/Infra.hpp"
#include "inner/coroutine.hpp"
#include "inner/RawOperator.hpp"

#include <functional>

struct GNUNET_CONFIGURATION_Handle;

namespace gnunetpp
{

struct Ego
{
    Ego(GNUNET_IDENTITY_Ego* ego);
    ~Ego() = default;

    const GNUNET_IDENTITY_PublicKey& publicKey() const;
    const GNUNET_IDENTITY_PrivateKey&  privateKey() const;
    GNUNET_IDENTITY_KeyType keyType() const;

    GNUNET_IDENTITY_Ego* native_handle() const { return ego; }
    GNUNET_IDENTITY_Ego* ego = nullptr;

    std::strong_ordering operator<=>(const Ego&) const;
    bool operator==(const Ego&) const = default;
    bool operator!=(const Ego&) const = default;
    bool operator<(const Ego&) const = default;
    bool operator>(const Ego&) const = default;
    bool operator<=(const Ego&) const = default;
    bool operator>=(const Ego&) const = default;

protected:
    GNUNET_IDENTITY_PublicKey pk;
    GNUNET_IDENTITY_PrivateKey sk;
};

struct IdentityService : public Service
{
    IdentityService(const GNUNET_CONFIGURATION_Handle* cfg);
    ~IdentityService();

    void shutdown() override;

    /**
     * @brief Create a new identity
     * 
     * @param name Name of the new identity
     * @param fn Callback to call when the identity is created or an error occurs
     * @param type Key type to use for the identity. Defaults to ECDSA.
     * @return GNUNET_IDENTITY_Operation* handle to the operation
     */
    GNUNET_IDENTITY_Operation* createIdentity(const std::string& name
        , std::function<void(const GNUNET_IDENTITY_PrivateKey*, const std::string&)> fn
        , GNUNET_IDENTITY_KeyType type = GNUNET_IDENTITY_TYPE_ECDSA);
    cppcoro::task<const GNUNET_IDENTITY_PrivateKey*> createIdentity(const std::string& name
        , GNUNET_IDENTITY_KeyType type = GNUNET_IDENTITY_TYPE_ECDSA);

    /**
     * @brief Delete an identity
     * 
     * @param name Name of the identity to delete
     * @param fn Callback to call when the identity is deleted or an error occurs
     * @return GNUNET_IDENTITY_Operation*  handle to the operation
     */
    GNUNET_IDENTITY_Operation* deleteIdentity(const std::string& name
        , std::function<void(const std::string&)> fn);
    cppcoro::task<> deleteIdentity(const std::string& name);

    GNUNET_IDENTITY_Handle* native_handle() const { return handle; }
    GNUNET_IDENTITY_Handle* handle = nullptr;
};

std::string to_string(const GNUNET_IDENTITY_PrivateKey& key);
std::string to_string(const GNUNET_IDENTITY_PublicKey& key);
std::string to_string(GNUNET_IDENTITY_KeyType key);

/**
 * @brief Get all identities on the local node
 * 
 * @param cfg handle to the configuration
 * @param fn callback to call for each identity
 */
void getIdentities(const GNUNET_CONFIGURATION_Handle* cfg, std::function<void(const std::string&, GNUNET_IDENTITY_Ego* ego)> fn);

/**
 * @brief Get public key from private key
 * 
 * @param key private key
 */
GNUNET_IDENTITY_PublicKey getPublicKey(const GNUNET_IDENTITY_PrivateKey& key);
/**
 * @brief Get an Ego by name
 * 
 * @param cfg handle to GNUnet
 * @param name name of the ego
 * @param fn callback to call when the ego is found (passes nullptr if not found)
 * @return GNUNET_IDENTITY_EgoLookup* handle to the lookup operation
 */
GNUNET_IDENTITY_EgoLookup* getEgo(const GNUNET_CONFIGURATION_Handle* cfg
    , const std::string& name, std::function<void(std::optional<Ego>)> fn);
cppcoro::task<std::optional<Ego>> getEgo(const GNUNET_CONFIGURATION_Handle* cfg
    , const std::string& name);
/**
 * @brief Get public key from ego
 * 
 * @param ego ego to get the public key from
 */
GNUNET_IDENTITY_PublicKey getPublicKey(GNUNET_IDENTITY_Ego* ego);

/**
 * @brief Get the private key from an ego
 * 
 * @param ego The ego to get the private key from
 */
const GNUNET_IDENTITY_PrivateKey* getPrivateKey(const GNUNET_IDENTITY_Ego* ego);

/**
 * @brief Get an anonymous ego
 */
Ego anonymousEgo();

/**
 * @brief Get the key type of an ego. Could be ECDSA or EdDSA
 * 
 * @param ego The ego to get the key type from
 */
GNUNET_IDENTITY_KeyType getKeyType(GNUNET_IDENTITY_Ego* ego);

}

GNUNETPP_OPERATOR_COMPOARE_RAW_DATA(GNUNET_IDENTITY_PublicKey)
GNUNETPP_OPERATOR_COMPOARE_RAW_DATA(GNUNET_IDENTITY_PrivateKey)
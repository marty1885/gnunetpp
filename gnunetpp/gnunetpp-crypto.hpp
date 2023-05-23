#pragma once

#include <compare>
#include <gnunet/gnunet_util_lib.h>
#include <gnunet/gnunet_signatures.h>

#include <string>
#include <vector>
#include "inner/RawOperator.hpp"

namespace gnunetpp::crypto
{
GNUNET_HashCode hash(void* data, size_t size);
inline GNUNET_HashCode hash(const std::string_view& data)
{
    return hash(const_cast<char*>(data.data()), data.size());
}

GNUNET_HashCode hmac(const std::string_view key, void* data, size_t size);
GNUNET_HashCode hmac(const GNUNET_CRYPTO_AuthKey& key, void* data, size_t size);
inline GNUNET_HashCode hmac(const std::string_view key, const std::string_view& data)
{
    return hmac(key, const_cast<char*>(data.data()), data.size());
}
inline GNUNET_HashCode hmac(const GNUNET_CRYPTO_AuthKey& key, const std::string_view& data)
{
    return hmac(key, const_cast<char*>(data.data()), data.size());
}

GNUNET_HashCode randomHash(GNUNET_CRYPTO_Quality quality = GNUNET_CRYPTO_QUALITY_STRONG);
GNUNET_ShortHashCode randomShortHash(GNUNET_CRYPTO_Quality quality = GNUNET_CRYPTO_QUALITY_STRONG);
GNUNET_HashCode zeroHash();
GNUNET_ShortHashCode zeroShortHash();
std::vector<uint8_t> randomBytes(size_t size, GNUNET_CRYPTO_Quality quality = GNUNET_CRYPTO_QUALITY_STRONG);
uint64_t randomU64(GNUNET_CRYPTO_Quality quality = GNUNET_CRYPTO_QUALITY_STRONG);


std::string to_string(const GNUNET_HashCode& hash);
std::string to_string_full(const GNUNET_HashCode& hash);
std::string to_string(const GNUNET_CRYPTO_EcdsaPrivateKey& key);
std::string to_string(const GNUNET_CRYPTO_EcdsaPublicKey& key);
std::string to_string(const GNUNET_CRYPTO_EddsaPrivateKey& key);
std::string to_string(const GNUNET_CRYPTO_EddsaPublicKey& key);

GNUNET_HashCode hashCode(const std::string_view& data);

GNUNET_CRYPTO_EcdsaPublicKey getPublicKey(const GNUNET_CRYPTO_EcdsaPrivateKey& key);
GNUNET_CRYPTO_EddsaPublicKey getPublicKey(const GNUNET_CRYPTO_EddsaPrivateKey& key);
GNUNET_PeerIdentity myPeerIdentity(const GNUNET_CONFIGURATION_Handle *cfg);
GNUNET_CRYPTO_EddsaPrivateKey myPeerPrivateKey(const GNUNET_CONFIGURATION_Handle *cfg);

std::string to_string(const GNUNET_PeerIdentity& peer);
GNUNET_PeerIdentity peerIdentity(const std::string_view& str);

GNUNET_CRYPTO_EcdsaPrivateKey anonymousKey();

std::optional<GNUNET_CRYPTO_EcdsaSignature> sign(const GNUNET_CRYPTO_EcdsaPrivateKey& key, const std::string_view& data, uint32_t purpose = GNUNET_SIGNATURE_PURPOSE_TEST);
std::optional<GNUNET_CRYPTO_EddsaSignature> sign(const GNUNET_CRYPTO_EddsaPrivateKey& key, const std::string_view& data, uint32_t purpose = GNUNET_SIGNATURE_PURPOSE_TEST);
bool verify(const GNUNET_CRYPTO_EcdsaPublicKey& key, const std::string_view& data, const GNUNET_CRYPTO_EcdsaSignature& signature, uint32_t purpose = GNUNET_SIGNATURE_PURPOSE_TEST);
bool verify(const GNUNET_CRYPTO_EddsaPublicKey& key, const std::string_view& data, const GNUNET_CRYPTO_EddsaSignature& signature, uint32_t purpose = GNUNET_SIGNATURE_PURPOSE_TEST);

std::string base64Encode(const void* data, size_t size);
void base64Decode(const void* data, size_t size, void* out);

GNUNET_CRYPTO_EddsaPrivateKey generateEdDSAPrivateKey();
GNUNET_CRYPTO_EcdsaPrivateKey generateECDSAPrivateKey();
}

#define GNUNETPP_BASE64_CODEC(type) \
    inline std::string base64Encode(const type& data) \
    { \
        return gnunetpp::crypto::base64Encode(&data, sizeof(data)); \
    } \
    inline void base64Decode(const std::string_view& data, type& out) \
    { \
        gnunetpp::crypto::base64Decode(data.data(), data.size(), &out); \
    }

#define GNUNETPP_OPERATION_XOR(type) \
    inline type operator^(const type& a, const type& b) \
    { \
        type ret; \
        for(size_t i = 0; i < sizeof(type)/sizeof(a.bits[0]); i++) \
            ret.bits[i] = a.bits[i] ^ b.bits[i]; \
        return ret; \
    } \
    inline type operator ^=(type& a, const type& b) \
    { \
        for(size_t i = 0; i < sizeof(type)/sizeof(a.bits[0]); i++) \
            a.bits[i] ^= b.bits[i]; \
        return a; \
    }

GNUNETPP_BASE64_CODEC(GNUNET_HashCode)
GNUNETPP_BASE64_CODEC(GNUNET_ShortHashCode)
GNUNETPP_BASE64_CODEC(GNUNET_CRYPTO_EcdsaPrivateKey)
GNUNETPP_BASE64_CODEC(GNUNET_CRYPTO_EcdsaPublicKey)
GNUNETPP_BASE64_CODEC(GNUNET_PeerIdentity)
GNUNETPP_BASE64_CODEC(GNUNET_CRYPTO_EddsaPrivateKey)
GNUNETPP_BASE64_CODEC(GNUNET_CRYPTO_EddsaPublicKey)

GNUNETPP_OPERATION_XOR(GNUNET_HashCode)
GNUNETPP_OPERATION_XOR(GNUNET_ShortHashCode)

GNUNETPP_OPERATOR_COMPOARE_RAW_DATA(GNUNET_HashCode)
GNUNETPP_OPERATOR_COMPOARE_RAW_DATA(GNUNET_ShortHashCode)
GNUNETPP_OPERATOR_COMPOARE_RAW_DATA(GNUNET_CRYPTO_EcdsaPrivateKey)
GNUNETPP_OPERATOR_COMPOARE_RAW_DATA(GNUNET_CRYPTO_EcdsaPublicKey)
GNUNETPP_OPERATOR_COMPOARE_RAW_DATA(GNUNET_PeerIdentity)
GNUNETPP_OPERATOR_COMPOARE_RAW_DATA(GNUNET_CRYPTO_EddsaPrivateKey)
GNUNETPP_OPERATOR_COMPOARE_RAW_DATA(GNUNET_CRYPTO_EddsaPublicKey)

GNUNETPP_RAW_DATA_HASH(GNUNET_HashCode)
GNUNETPP_RAW_DATA_HASH(GNUNET_ShortHashCode)
GNUNETPP_RAW_DATA_HASH(GNUNET_CRYPTO_EcdsaPrivateKey)
GNUNETPP_RAW_DATA_HASH(GNUNET_CRYPTO_EcdsaPublicKey)
GNUNETPP_RAW_DATA_HASH(GNUNET_PeerIdentity)
GNUNETPP_RAW_DATA_HASH(GNUNET_CRYPTO_EddsaPrivateKey)
GNUNETPP_RAW_DATA_HASH(GNUNET_CRYPTO_EddsaPublicKey)

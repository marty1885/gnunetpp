#pragma once

#include <gnunet/gnunet_util_lib.h>

#include <string>
#include <vector>

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

GNUNET_HashCode random_hash(GNUNET_CRYPTO_Quality quality = GNUNET_CRYPTO_QUALITY_STRONG);
std::vector<uint8_t> random_bytes(size_t size, GNUNET_CRYPTO_Quality quality = GNUNET_CRYPTO_QUALITY_STRONG);
uint64_t random_u64(GNUNET_CRYPTO_Quality quality = GNUNET_CRYPTO_QUALITY_STRONG);


std::string to_string(const GNUNET_HashCode& hash);
std::string to_string_full(const GNUNET_HashCode& hash);
std::string to_string(const GNUNET_CRYPTO_EcdsaPrivateKey& key);
std::string to_string(const GNUNET_CRYPTO_EcdsaPublicKey& key);
std::string to_string(const GNUNET_CRYPTO_EddsaPrivateKey& key);
std::string to_string(const GNUNET_CRYPTO_EddsaPublicKey& key);

GNUNET_CRYPTO_EcdsaPublicKey get_public_key(const GNUNET_CRYPTO_EcdsaPrivateKey& key);
}

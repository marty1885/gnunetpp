#include "gnunetpp-crypto.hpp"

#include <mutex>

namespace gnunetpp::crypto
{

// GNUnet uses a shared buffer for hash to string conversion. We need to protect it.
static std::mutex g_mtx_hash;
static std::mutex g_mtx_hash_full;

GNUNET_HashCode hash(void* data, size_t size)
{
    GNUNET_HashCode hash;
    GNUNET_CRYPTO_hash(data, size, &hash);
    return hash;
}

GNUNET_HashCode hmac(const std::string_view key, void* data, size_t size)
{
    GNUNET_HashCode hash;
    GNUNET_CRYPTO_hmac_raw(key.data(), key.size(), data, size, &hash);
    return hash;
}

GNUNET_HashCode hmac(const GNUNET_CRYPTO_AuthKey& key, void* data, size_t size)
{
    GNUNET_HashCode hash;
    GNUNET_CRYPTO_hmac(&key, data, size, &hash);
    return hash;
}

GNUNET_HashCode randomHash(GNUNET_CRYPTO_Quality quality)
{
    GNUNET_HashCode hash;
    GNUNET_CRYPTO_hash_create_random(quality, &hash);
    return hash;
}

std::vector<uint8_t> randomBytes(size_t size, GNUNET_CRYPTO_Quality quality)
{
    std::vector<uint8_t> bytes(size);
    GNUNET_CRYPTO_random_block(quality, bytes.data(), size);
    return bytes;
}

uint64_t randomU64(GNUNET_CRYPTO_Quality quality)
{
    return GNUNET_CRYPTO_random_u64(quality, UINT64_MAX);
}

std::string to_string(const GNUNET_HashCode& hash)
{
    std::lock_guard<std::mutex> m(g_mtx_hash);
    const char* str = GNUNET_h2s(&hash);
    std::string ret{str};
    return ret;
}

std::string to_string_full(const GNUNET_HashCode& hash)
{
    std::lock_guard<std::mutex> m(g_mtx_hash_full);
    const char* str = GNUNET_h2s_full(&hash);
    std::string ret{str};
    return ret;
}

std::string to_string(const GNUNET_CRYPTO_EcdsaPrivateKey& key)
{
    GNUNET_CRYPTO_EcdsaPublicKey pub;
    char* privs = GNUNET_CRYPTO_ecdsa_private_key_to_string(&key);
    std::string res(privs);
    GNUNET_free(privs);
    return res;
}

std::string to_string(const GNUNET_CRYPTO_EcdsaPublicKey& key)
{
    char* pubs = GNUNET_CRYPTO_ecdsa_public_key_to_string(&key);
    std::string res(pubs);
    GNUNET_free(pubs);
    return res;
}

std::string to_string(const GNUNET_CRYPTO_EddsaPrivateKey& key)
{
    GNUNET_CRYPTO_EddsaPublicKey pub;
    char* privs = GNUNET_CRYPTO_eddsa_private_key_to_string(&key);
    std::string res(privs);
    GNUNET_free(privs);
    return res;
}

std::string to_string(const GNUNET_CRYPTO_EddsaPublicKey& key)
{
    char* pubs = GNUNET_CRYPTO_eddsa_public_key_to_string(&key);
    std::string res(pubs);
    GNUNET_free(pubs);
    return res;
}

GNUNET_CRYPTO_EcdsaPublicKey getPublicKey(const GNUNET_CRYPTO_EcdsaPrivateKey& key)
{
    GNUNET_CRYPTO_EcdsaPublicKey pub;
    GNUNET_CRYPTO_ecdsa_key_get_public(&key, &pub);
    return pub;
}

GNUNET_PeerIdentity myPeerIdentity(const GNUNET_CONFIGURATION_Handle *cfg)
{
    GNUNET_PeerIdentity id;
    if(GNUNET_CRYPTO_get_peer_identity(cfg, &id) == GNUNET_OK)
        return id;
    throw std::runtime_error("Failed to get host peer identity");
}

std::string to_string(const GNUNET_PeerIdentity& id)
{
    const char* str = GNUNET_i2s_full(&id);
    GNUNET_assert(str != nullptr);
    return str;
}

GNUNET_PeerIdentity peerIdentity(const std::string_view& str)
{
    GNUNET_PeerIdentity id;
    if(GNUNET_OK != GNUNET_CRYPTO_eddsa_public_key_from_string(str.data(), str.size(), &id.public_key))
        throw std::runtime_error("Failed to parse peer identity");
    return id;
}

GNUNET_HashCode zeroHash()
{
    GNUNET_HashCode hash;
    memset(&hash, 0, sizeof(hash));
    return hash;
}

GNUNET_CRYPTO_EcdsaPrivateKey anonymousKey()
{
    return *GNUNET_CRYPTO_ecdsa_key_get_anonymous();
}

std::optional<GNUNET_CRYPTO_EcdsaSignature> sign(const GNUNET_CRYPTO_EcdsaPrivateKey& key, const std::string_view& data, uint32_t purpose)
{
    GNUNET_CRYPTO_EcdsaSignature sig;
    std::vector<uint8_t> chunk(data.size() + sizeof(GNUNET_CRYPTO_EccSignaturePurpose));
    GNUNET_CRYPTO_EccSignaturePurpose header;
    header.purpose = htonl(purpose);
    header.size = htonl(data.size());
    memcpy(chunk.data(), &header, sizeof(header));
    memcpy(chunk.data() + sizeof(header), data.data(), data.size());
    if(GNUNET_OK != GNUNET_CRYPTO_ecdsa_sign_(&key, (GNUNET_CRYPTO_EccSignaturePurpose*)chunk.data(), &sig))
        return std::nullopt;
    return sig;
}

std::optional<GNUNET_CRYPTO_EddsaSignature> sign(const GNUNET_CRYPTO_EddsaPrivateKey& key, const std::string_view& data, uint32_t purpose)
{
    GNUNET_CRYPTO_EddsaSignature sig;
    std::vector<uint8_t> chunk(data.size() + sizeof(GNUNET_CRYPTO_EccSignaturePurpose));
    GNUNET_CRYPTO_EccSignaturePurpose header;
    header.purpose = htonl(purpose);
    header.size = htonl(data.size());
    memcpy(chunk.data(), &header, sizeof(header));
    memcpy(chunk.data() + sizeof(header), data.data(), data.size());
    if(GNUNET_OK != GNUNET_CRYPTO_eddsa_sign_(&key, (GNUNET_CRYPTO_EccSignaturePurpose*)chunk.data(), &sig))
        return std::nullopt;
    return sig;
}

bool verify(const GNUNET_CRYPTO_EcdsaPublicKey& key, const std::string_view& data, const GNUNET_CRYPTO_EcdsaSignature& signature, uint32_t purpose)
{
    std::vector<uint8_t> chunk(data.size() + sizeof(GNUNET_CRYPTO_EccSignaturePurpose));
    GNUNET_CRYPTO_EccSignaturePurpose header;
    header.purpose = htonl(purpose);
    header.size = htonl(data.size());
    memcpy(chunk.data(), &header, sizeof(header));
    memcpy(chunk.data() + sizeof(header), data.data(), data.size());
    return GNUNET_OK == GNUNET_CRYPTO_ecdsa_verify_(purpose, (GNUNET_CRYPTO_EccSignaturePurpose*)chunk.data(), &signature, &key);
}

bool verify(const GNUNET_CRYPTO_EddsaPublicKey& key, const std::string_view& data, const GNUNET_CRYPTO_EddsaSignature& signature, uint32_t purpose)
{
    std::vector<uint8_t> chunk(data.size() + sizeof(GNUNET_CRYPTO_EccSignaturePurpose));
    GNUNET_CRYPTO_EccSignaturePurpose header;
    header.purpose = htonl(purpose);
    header.size = htonl(data.size());
    memcpy(chunk.data(), &header, sizeof(header));
    memcpy(chunk.data() + sizeof(header), data.data(), data.size());
    return GNUNET_OK == GNUNET_CRYPTO_eddsa_verify_(purpose, (GNUNET_CRYPTO_EccSignaturePurpose*)chunk.data(), &signature, &key);
}
}
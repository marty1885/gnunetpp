#pragma once

#include <gnunet/gnunet_datastore_service.h>

#include "inner/Infra.hpp"
#include "inner/coroutine.hpp"
#include "gnunetpp-crypto.hpp"

namespace gnunetpp
{
struct DataStore : public Service
{
    DataStore(const GNUNET_CONFIGURATION_Handle* cfg);
    virtual ~DataStore();
    virtual void shutdown() override;

    void put(const GNUNET_HashCode& key
        , const void* data, size_t data_size
        , std::function<void(std::optional<std::string> error)> completedCallback
        , std::chrono::seconds expiration = std::chrono::years(1)
        // Default "magic" values taken from various GNUnet examples. Seems to be what GNUnet likes
        , uint32_t priority = 42
        , uint32_t replication = 0
        , uint32_t anonymity = 0
        , GNUNET_BLOCK_Type type = GNUNET_BLOCK_TYPE_TEST
        , uint32_t queue_priority = 2
        , uint32_t max_queue_size = 1);
    void getOne(const GNUNET_HashCode& hash, std::function<void(std::optional<std::vector<uint8_t>>, uint64_t)> callback
        , uint32_t queue_priority=1, uint32_t max_queue_size=1, GNUNET_BLOCK_Type type = GNUNET_BLOCK_TYPE_TEST
        , uint64_t uid = 0);
    void getOne(const GNUNET_HashCode& hash, std::function<void(std::optional<std::vector<uint8_t>>)> callback
        , uint32_t queue_priority=1, uint32_t max_queue_size=1, GNUNET_BLOCK_Type type = GNUNET_BLOCK_TYPE_TEST
        , uint64_t uid = 0)
    {
        auto functor = [callback=std::move(callback)](std::optional<std::vector<uint8_t>> data, uint64_t uid) {
            callback(data);
        };
        getOne(hash, std::move(functor), queue_priority, max_queue_size, type, uid);
    }
    void getOne(const std::string_view key, std::function<void(std::optional<std::vector<uint8_t>>, uint64_t)> callback
        , uint32_t queue_priority=1, uint32_t max_queue_size=1, GNUNET_BLOCK_Type type = GNUNET_BLOCK_TYPE_TEST
        , uint64_t uid = 0)
    {
        auto hash = gnunetpp::crypto::hash(key);
        getOne(hash, std::move(callback), queue_priority, max_queue_size, type, uid);
    }
    void getOne(const std::string_view key, std::function<void(std::optional<std::vector<uint8_t>>)> callback
        , uint32_t queue_priority=1, uint32_t max_queue_size=1, GNUNET_BLOCK_Type type = GNUNET_BLOCK_TYPE_TEST
        , uint64_t uid = 0)
    {
        auto hash = gnunetpp::crypto::hash(key);
        getOne(hash, std::move(callback), queue_priority, max_queue_size, type, uid);
    }
    
    cppcoro::task<> put(const GNUNET_HashCode& key
        , const void* data, size_t data_size
        , std::chrono::seconds expiration = std::chrono::years(1)
        // Default "magic" values taken from various GNUnet examples. Seems to be what GNUnet likes
        , uint32_t priority = 42
        , uint32_t replication = 0
        , uint32_t anonymity = 0
        , GNUNET_BLOCK_Type type = GNUNET_BLOCK_TYPE_TEST
        , uint32_t queue_priority = 2
        , uint32_t max_queue_size = 1);
    cppcoro::task<> put(const GNUNET_HashCode& key
        , const std::string_view sv
        , std::chrono::seconds expiration = std::chrono::years(1)
        // Default "magic" values taken from various GNUnet examples. Seems to be what GNUnet likes
        , uint32_t priority = 42
        , uint32_t replication = 0
        , uint32_t anonymity = 0
        , GNUNET_BLOCK_Type type = GNUNET_BLOCK_TYPE_TEST
        , uint32_t queue_priority = 2
        , uint32_t max_queue_size = 1)
    {
        return put(key, sv.data(), sv.size(), expiration, priority, replication, anonymity, type, queue_priority, max_queue_size);
    }
    cppcoro::task<> put(const GNUNET_HashCode& key
        , const std::vector<uint8_t>& data
        , std::chrono::seconds expiration = std::chrono::years(1)
        // Default "magic" values taken from various GNUnet examples. Seems to be what GNUnet likes
        , uint32_t priority = 42
        , uint32_t replication = 0
        , uint32_t anonymity = 0
        , GNUNET_BLOCK_Type type = GNUNET_BLOCK_TYPE_TEST
        , uint32_t queue_priority = 2
        , uint32_t max_queue_size = 1)
    {
        return put(key, data.data(), data.size(), expiration, priority, replication, anonymity, type, queue_priority, max_queue_size);
    }
    
    cppcoro::task<> put(const std::string_view key
        , const void* data, size_t data_size
        , std::chrono::seconds expiration = std::chrono::years(1)
        // Default "magic" values taken from various GNUnet examples. Seems to be what GNUnet likes
        , uint32_t priority = 42
        , uint32_t replication = 0
        , uint32_t anonymity = 0
        , GNUNET_BLOCK_Type type = GNUNET_BLOCK_TYPE_TEST
        , uint32_t queue_priority = 2
        , uint32_t max_queue_size = 1)
    {
        auto hash = crypto::hash(key);
        return put(hash, data, data_size, expiration, priority, replication, anonymity, type, queue_priority, max_queue_size);
    }
    cppcoro::task<> put(const std::string_view key
        , const std::string_view sv
        , std::chrono::seconds expiration = std::chrono::years(1)
        // Default "magic" values taken from various GNUnet examples. Seems to be what GNUnet likes
        , uint32_t priority = 42
        , uint32_t replication = 0
        , uint32_t anonymity = 0
        , GNUNET_BLOCK_Type type = GNUNET_BLOCK_TYPE_TEST
        , uint32_t queue_priority = 2
        , uint32_t max_queue_size = 1)
    {
        auto hash = crypto::hash(key);
        return put(hash, sv.data(), sv.size(), expiration, priority, replication, anonymity, type, queue_priority, max_queue_size);
    }
    cppcoro::task<> put(const std::string_view key
        , const std::vector<uint8_t>& data
        , std::chrono::seconds expiration = std::chrono::years(1)
        // Default "magic" values taken from various GNUnet examples. Seems to be what GNUnet likes
        , uint32_t priority = 42
        , uint32_t replication = 0
        , uint32_t anonymity = 0
        , GNUNET_BLOCK_Type type = GNUNET_BLOCK_TYPE_TEST
        , uint32_t queue_priority = 2
        , uint32_t max_queue_size = 1)
    {
        auto hash = crypto::hash(key);
        return put(hash, data.data(), data.size(), expiration, priority, replication, anonymity, type, queue_priority, max_queue_size);
    }
    
    cppcoro::task<std::optional<std::vector<uint8_t>>> getOne(const GNUNET_HashCode& hash
        , uint32_t queue_priority=1, uint32_t max_queue_size=1, GNUNET_BLOCK_Type type = GNUNET_BLOCK_TYPE_TEST
        , uint64_t uid = 0);
    cppcoro::task<std::optional<std::vector<uint8_t>>> getOne(const std::string key
        , uint32_t queue_priority=1, uint32_t max_queue_size=1, GNUNET_BLOCK_Type type = GNUNET_BLOCK_TYPE_TEST
        , uint64_t uid = 0)
    {
        auto hash = gnunetpp::crypto::hash(key);
        return getOne(hash, queue_priority, max_queue_size, type, uid);
    }
    cppcoro::async_generator<std::vector<uint8_t>> get(const GNUNET_HashCode& hash
        , uint32_t queue_priority=1, uint32_t max_queue_size=1, GNUNET_BLOCK_Type type = GNUNET_BLOCK_TYPE_TEST);
    cppcoro::async_generator<std::vector<uint8_t>> get(const std::string key
        , uint32_t queue_priority=1, uint32_t max_queue_size=1, GNUNET_BLOCK_Type type = GNUNET_BLOCK_TYPE_TEST)
    {
        auto hash = gnunetpp::crypto::hash(key);
        return get(hash, queue_priority, max_queue_size, type);
    }
    

    GNUNET_DATASTORE_Handle* datastore;
};
}
#pragma once

#include <gnunet/gnunet_core_service.h>
#include <gnunet/gnunet_dht_service.h>

#include <gnunetpp-dht.hpp>
#include <gnunetpp-scheduler.hpp>
#include <gnunetpp-crypto.hpp>
#include "inner/Infra.hpp"

#include <chrono>
#include <stdexcept>
#include <functional>
#include <cassert>
#include <set>
#include <memory>


namespace gnunetpp
{
struct DHT : public Service
{
    using PutCallbackFunctor = std::function<void()>;
    using GetCallbackFunctor = std::function<bool(std::string_view)>;

    struct GetCallbackPack
    {
        DHT* self;
        GNUNET_DHT_GetHandle* handle;
        TaskID timer_task;
        GetCallbackFunctor callback;
    };

    DHT(const GNUNET_CONFIGURATION_Handle* cfg, unsigned int ht_len = 32)
    {
        dht_handle = GNUNET_DHT_connect(cfg, ht_len);
        if(dht_handle == NULL)
            throw std::runtime_error("Failed to connect to GNUNet DHT service");
        registerService(this);
    }

    ~DHT()
    {
        shutdown();
        removeService(this);
    }

    void shutdown() override;
    GNUNET_DHT_PutHandle* put(const std::string_view key, const std::string_view data
        , PutCallbackFunctor completedCallback
        , std::chrono::duration<double> expiration = std::chrono::hours(1)
        , unsigned int replication = 5
        , GNUNET_BLOCK_Type data_type = GNUNET_BLOCK_TYPE_TEST
        , GNUNET_DHT_RouteOption routing_options = GNUNET_DHT_RO_NONE);

    GNUNET_DHT_GetHandle* get(const std::string_view key, GetCallbackFunctor completedCallback
        , std::chrono::duration<double> search_timeout = std::chrono::seconds(10)
        , GNUNET_BLOCK_Type data_type = GNUNET_BLOCK_TYPE_TEST
        , unsigned int replication = 5
        , GNUNET_DHT_RouteOption routing_options = GNUNET_DHT_RO_NONE);

    void cancle(GNUNET_DHT_PutHandle* handle);
    void cancle(GNUNET_DHT_GetHandle* handle);

    GNUNET_DHT_Handle* native_handle() const
    {
        return dht_handle;
    }

protected:
    static void putCallback(void* cls)
    {
        // ensure functor is deleted even if it throws
        std::unique_ptr<PutCallbackFunctor> functor{static_cast<PutCallbackFunctor*>(cls)};
        assert(functor != nullptr);
        (*functor)();
    }

    static void getCallback(void *cls,
                           struct GNUNET_TIME_Absolute exp,
                           const struct GNUNET_HashCode *query_hash,
                           const struct GNUNET_PeerIdentity *trunc_peer,
                           const struct GNUNET_DHT_PathElement *get_path,
                           unsigned int get_path_length,
                           const struct GNUNET_DHT_PathElement *put_path,
                           unsigned int put_path_length,
                           enum GNUNET_BLOCK_Type type,
                           size_t size,
                           const void *data);

    GNUNET_DHT_Handle *dht_handle = nullptr;
    std::set<GetCallbackPack*> get_handles;
};

}

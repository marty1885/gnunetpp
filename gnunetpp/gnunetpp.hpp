#pragma once

#include "gnunetpp-scheduler.hpp"
#include "gnunetpp-crypto.hpp"

#include "gnunet/platform.h"
#include <gnunet/gnunet_core_service.h>
#include "gnunet/gnunet_dht_service.h"

#include <chrono>
#include <stdexcept>
#include <functional>
#include <cassert>
#include <memory>
#include <mutex>
#include <set>
#include <iostream>
#include <vector>

namespace gnunetpp
{

struct Service
{
    virtual ~Service() = default;
    Service(const Service&) = delete;
    Service& operator=(const Service&) = delete;
    Service(Service&&) = delete;
    Service& operator=(Service&&) = delete;

    virtual void shutdown() = 0;
protected:
    Service() = default;
};

std::mutex g_mtx_services;
std::set<Service*> g_services;

inline void registerService(Service* service)
{
    std::lock_guard<std::mutex> m(g_mtx_services);
    g_services.insert(service);
}

inline void removeService(Service* service)
{
    std::lock_guard<std::mutex> m(g_mtx_services);
    service->shutdown();
    g_services.erase(service);
}

inline void removeAllServices()
{
    std::lock_guard<std::mutex> m(g_mtx_services);
    for (auto& service : g_services)
        service->shutdown();
    g_services.clear();
}

struct DHT : public Service
{
    using PutCallbackFunctor = std::function<void()>;
    using GetCallbackFunctor = std::function<bool(std::string_view)>;

    struct GetCallbackPack
    {
        DHT* self;
        GNUNET_DHT_GetHandle* handle;
        scheduler::TaskID timer_task;
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

    void shutdown() override
    {
        if(dht_handle != NULL)
        {
            for(auto pack : get_handles) {
                GNUNET_DHT_get_stop(pack->handle);
                scheduler::cancel(pack->timer_task);
                delete pack;
            }
            get_handles.clear();

            GNUNET_DHT_disconnect(dht_handle);
            dht_handle = NULL;
        }
    }

    GNUNET_DHT_PutHandle* put(const std::string_view key, const std::string_view data
        , PutCallbackFunctor completedCallback
        , std::chrono::duration<double> expiration = std::chrono::hours(1)
        , unsigned int replication = 5
        , GNUNET_BLOCK_Type data_type = GNUNET_BLOCK_TYPE_TEST
        , GNUNET_DHT_RouteOption routing_options = GNUNET_DHT_RO_NONE)
    {
        if(dht_handle == NULL)
            throw std::runtime_error("DHT not connected");
        GNUNET_HashCode key_hash = crypto::hash(key);

        // XXX: This only works because internally GNUNet uses msec. If they ever change this, this will break.
        const size_t num_usecs = std::chrono::duration_cast<std::chrono::microseconds>(expiration).count();
        GNUNET_TIME_Relative gnunet_expiration{num_usecs};

        PutCallbackFunctor* functor = new PutCallbackFunctor(std::move(completedCallback));

        // No need to copy data to ensure lifetime ourselves, GNUNet does it for us
        GNUNET_DHT_PutHandle* handle = GNUNET_DHT_put(dht_handle, &key_hash, replication, routing_options, data_type, data.size()
            , data.data(), GNUNET_TIME_relative_to_absolute(gnunet_expiration), &DHT::putCallback, functor);
        if(handle == NULL)
            throw std::runtime_error("Failed to put data into GNUNet DHT");
        return handle;
    }

    GNUNET_DHT_GetHandle* get(const std::string_view key, GetCallbackFunctor completedCallback
        , std::chrono::duration<double> search_timeout = std::chrono::seconds(10)
        , GNUNET_BLOCK_Type data_type = GNUNET_BLOCK_TYPE_TEST
        , unsigned int replication = 5
        , GNUNET_DHT_RouteOption routing_options = GNUNET_DHT_RO_NONE)
    {
        if(dht_handle == NULL)
            throw std::runtime_error("DHT not connected");
        GNUNET_HashCode key_hash = crypto::hash(key);
        auto data = new GetCallbackPack;
        data->callback = std::move(completedCallback);
        data->self = this;

        GNUNET_DHT_GetHandle* handle = GNUNET_DHT_get_start(dht_handle, data_type, &key_hash, replication, routing_options
            , NULL, 0, &DHT::getCallback, data);
        data->handle = handle;
        if(handle == NULL)
            throw std::runtime_error("Failed to get data from GNUNet DHT");
        get_handles.insert(data);
        data->timer_task = scheduler::runLater(search_timeout, [data] () {
            auto self = data->self;
            if(self->get_handles.find(data) != self->get_handles.end())
            {
                GNUNET_DHT_get_stop(data->handle);
                delete data;
                self->get_handles.erase(data);
            }
        });
        return handle;
    }

    void cancle(GNUNET_DHT_PutHandle* handle)
    {
        GNUNET_DHT_put_cancel(handle);
    }

    void cancle(GNUNET_DHT_GetHandle* handle)
    {
        GNUNET_DHT_get_stop(handle);
        // HACK: 
        for(auto pack : get_handles) {
            if(pack->handle == handle) {
                scheduler::cancel(pack->timer_task);
                delete pack;
                get_handles.erase(pack);
                break;
            }
        }
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
                           const void *data)
    {
        auto pack = reinterpret_cast<GetCallbackPack*>(cls);
        assert(pack != nullptr);
        std::string_view data_view{reinterpret_cast<const char*>(data), size};
        bool keep_running = pack->callback(data_view);
        if(keep_running == false) {
            pack->self->get_handles.erase(pack);
            GNUNET_DHT_get_stop(pack->handle);
            scheduler::cancel(pack->timer_task);
            delete pack;
        }
    }

    struct GNUNET_DHT_Handle *dht_handle = nullptr;
    std::set<GetCallbackPack*> get_handles;
};

void run(std::function<void(const GNUNET_CONFIGURATION_Handle*)> f)
{
    using CallbackType = std::function<void(const GNUNET_CONFIGURATION_Handle* c)>;

    const char* args_dummy = "gnunetpp";
    CallbackType* functor = new CallbackType(std::move(f));
    struct GNUNET_GETOPT_CommandLineOption options[] = {
        GNUNET_GETOPT_OPTION_END
    };
    auto r = GNUNET_PROGRAM_run(1, const_cast<char**>(&args_dummy), "gnunetpp", "no help", options
        , [](void *cls, char *const *args, const char *cfgfile
            , const GNUNET_CONFIGURATION_Handle* c) {
                std::unique_ptr<CallbackType> functor{static_cast<CallbackType*>(cls)};
                assert(functor != nullptr);
                (*functor)(c);
            }
        , functor);
    if(r != GNUNET_OK)
        throw std::runtime_error("GNUNet program run failed");
}

void shutdown()
{
    GNUNET_SCHEDULER_add_now([] (void* d) {
        removeAllServices();
        scheduler::shutdown();
    }, NULL);
}

}
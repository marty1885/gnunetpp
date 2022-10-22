#include "gnunetpp-dht.hpp"

#include <iostream>

namespace gnunetpp
{
void DHT::shutdown()
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

GNUNET_DHT_PutHandle* DHT::put(const std::string_view key, const std::string_view data
    , PutCallbackFunctor completedCallback
    , std::chrono::duration<double> expiration
    , unsigned int replication
    , GNUNET_BLOCK_Type data_type
    , GNUNET_DHT_RouteOption routing_options)
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

GNUNET_DHT_GetHandle* DHT::get(const std::string_view key, GetCallbackFunctor completedCallback
    , std::chrono::duration<double> search_timeout
    , GNUNET_BLOCK_Type data_type
    , unsigned int replication
    , GNUNET_DHT_RouteOption routing_options)
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
    data->timer_task = scheduler::runLater(search_timeout, [data, this] () {
        GNUNET_DHT_get_stop(data->handle);
        delete data;
        get_handles.erase(data);
    });
    return handle;
}

void DHT::cancle(GNUNET_DHT_PutHandle* handle)
{
    GNUNET_DHT_put_cancel(handle);
}

void DHT::cancle(GNUNET_DHT_GetHandle* handle)
{
    GNUNET_DHT_get_stop(handle);
    // HACK: Need a faster way to find the GetCallbackPack
    for(auto pack : get_handles) {
        if(pack->handle == handle) {
            scheduler::cancel(pack->timer_task);
            delete pack;
            get_handles.erase(pack);
            break;
        }
    }
}
void DHT::getCallback(void *cls,
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
}
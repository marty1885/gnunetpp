#include "gnunetpp-dht.hpp"

#include <iostream>
#include <unordered_map>

namespace gnunetpp
{
void DHT::shutdown()
{
    if(dht_handle != NULL)
    {
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

cppcoro::task<> DHT::put(const std::string_view key, const std::string_view data
        , std::chrono::duration<double> expiration
        , unsigned int replication
        , GNUNET_BLOCK_Type data_type
        , GNUNET_DHT_RouteOption routing_options)
{
    struct PutAwaiter : public EagerAwaiter<>
    {
        PutAwaiter(DHT* dht, const std::string_view key, const std::string_view data
            , std::chrono::duration<double> expiration
            , unsigned int replication
            , GNUNET_BLOCK_Type data_type
            , GNUNET_DHT_RouteOption routing_options)
        {
            dht->put(key, data, [this] () {
                setValue();
            }, expiration, replication, data_type, routing_options);
        }
    };
    co_await PutAwaiter(this, key, data, expiration, replication, data_type, routing_options);
}

GNUNET_DHT_GetHandle* DHT::get(const std::string_view key, GetCallbackFunctor completedCallback
    , std::chrono::duration<double> search_timeout
    , GNUNET_BLOCK_Type data_type
    , unsigned int replication
    , GNUNET_DHT_RouteOption routing_options
    , std::function<void()> finished_callback)
{
    if(dht_handle == NULL)
        throw std::runtime_error("DHT not connected");
    GNUNET_HashCode key_hash = crypto::hash(key);
    auto data = new GetCallbackPack;

    GNUNET_DHT_GetHandle* handle = GNUNET_DHT_get_start(dht_handle, data_type, &key_hash, replication, routing_options
        , NULL, 0, &DHT::getCallback, data);
    if(handle == NULL) {
        delete data;
        throw std::runtime_error("Failed to get data from GNUNet DHT");
    }
    data->callback = std::move(completedCallback);
    data->handle = handle;
    data->timer_task = scheduler::runLater(search_timeout, [data, this] () {
        if(data->finished_callback)
            data->finished_callback();
        GNUNET_DHT_get_stop(data->handle);
        delete data;
    }, true);
    data->finished_callback = std::move(finished_callback);
    return handle;
}

cppcoro::async_generator<std::string> DHT::get(const std::string_view key
        , std::chrono::duration<double> search_timeout
        , GNUNET_BLOCK_Type data_type
        , unsigned int replication
        , GNUNET_DHT_RouteOption routing_options)

{
    EagerAwaiter<std::pair<std::string, bool>> awaiter;
    auto handle = get(key, [&awaiter] (std::string_view data) {
        std::cout << "Got data: " << data << std::endl;
        awaiter.setValue({std::string(data), true});
        return true;
    }, search_timeout, data_type, replication, routing_options
    , [&awaiter](){
        awaiter.setValue({std::string(), false});
    });

    while(true) {
        auto [result, have_data] = co_await awaiter;
        if(!have_data)
            break;
        co_yield result;
        awaiter = EagerAwaiter<std::pair<std::string, bool>>();
    }
}

void DHT::cancle(GNUNET_DHT_PutHandle* handle)
{
    GNUNET_DHT_put_cancel(handle);
}

void DHT::cancle(GNUNET_DHT_GetHandle* handle)
{
    //TODO: Fix leak caused by not deleting GetCallbackPack
    GNUNET_DHT_get_stop(handle);
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
        if(pack->finished_callback)
            pack->finished_callback();
        GNUNET_DHT_get_stop(pack->handle);
        scheduler::cancel(pack->timer_task);
        delete pack;
    }
}
}
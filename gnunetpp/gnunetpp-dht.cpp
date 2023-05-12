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
    auto key_hash = crypto::hash(key);
    return put(key_hash, data, std::move(completedCallback), expiration, replication, data_type, routing_options);
}

GNUNET_DHT_PutHandle* DHT::put(const GNUNET_HashCode& key_hash, const std::string_view data
    , PutCallbackFunctor completedCallback
    , std::chrono::duration<double> expiration
    , unsigned int replication
    , GNUNET_BLOCK_Type data_type
    , GNUNET_DHT_RouteOption routing_options)
{
    if(dht_handle == NULL)
        throw std::runtime_error("DHT not connected");

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
    return put(crypto::hash(key), data, expiration, replication, data_type, routing_options);
}

cppcoro::task<> DHT::put(GNUNET_HashCode key_hash, const std::string_view data
        , std::chrono::duration<double> expiration
        , unsigned int replication
        , GNUNET_BLOCK_Type data_type
        , GNUNET_DHT_RouteOption routing_options)
{
    struct PutAwaiter : public EagerAwaiter<>
    {
        PutAwaiter(DHT* dht, const GNUNET_HashCode& key_hash, const std::string_view data
            , std::chrono::duration<double> expiration
            , unsigned int replication
            , GNUNET_BLOCK_Type data_type
            , GNUNET_DHT_RouteOption routing_options)
        {
            dht->put(key_hash, data, [this] () {
                setValue();
            }, expiration, replication, data_type, routing_options);
        }
    };
    co_await PutAwaiter(this, key_hash, data, expiration, replication, data_type, routing_options);
}

DHT::GetCallbackPack* DHT::get(const std::string_view key, GetCallbackFunctor completedCallback
    , std::chrono::duration<double> search_timeout
    , GNUNET_BLOCK_Type data_type
    , unsigned int replication
    , GNUNET_DHT_RouteOption routing_options
    , std::function<void()> finished_callback)
{
    return get(crypto::hash(key), std::move(completedCallback), search_timeout
        , data_type, replication, routing_options, std::move(finished_callback));
}

DHT::GetCallbackPack* DHT::get(const GNUNET_HashCode& key_hash, GetCallbackFunctor completedCallback
    , std::chrono::duration<double> search_timeout
    , GNUNET_BLOCK_Type data_type
    , unsigned int replication
    , GNUNET_DHT_RouteOption routing_options
    , std::function<void()> finished_callback)
{
    if(dht_handle == NULL)
        throw std::runtime_error("DHT not connected");
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
    return data;
}

GeneratorWrapper<std::string> DHT::get(const std::string_view key
        , std::chrono::duration<double> search_timeout
        , GNUNET_BLOCK_Type data_type
        , unsigned int replication
        , GNUNET_DHT_RouteOption routing_options)
{
    return get(crypto::hash(key), search_timeout, data_type, replication, routing_options);
}

GeneratorWrapper<std::string> DHT::get(GNUNET_HashCode key_hash
        , std::chrono::duration<double> search_timeout
        , GNUNET_BLOCK_Type data_type
        , unsigned int replication
        , GNUNET_DHT_RouteOption routing_options)

{
    auto awaiter = std::make_unique<QueuedAwaiter<std::string>>();
    auto handle = get(key_hash, [awaiter=awaiter.get()] (std::string_view data) {
        awaiter->addValue(std::string(data));
        return true;
    }, search_timeout, data_type, replication, routing_options
    , [awaiter=awaiter.get()](){
        awaiter->finish();
    });
    if(handle == NULL)
        throw std::runtime_error("Failed to get data from GNUNet DHT");

    return GeneratorWrapper<std::string>(std::move(awaiter), [this, awaiter=awaiter.get(), handle] {
        handle->cancel();
    });
}

void DHT::GetCallbackPack::cancel()
{
    scheduler::cancel(timer_task);
    if(finished_callback)
        finished_callback();
    GNUNET_DHT_get_stop(handle);
    delete this;
}

void DHT::cancle(GNUNET_DHT_PutHandle* handle)
{
    GNUNET_DHT_put_cancel(handle);
}

void DHT::cancle(GetCallbackPack* handle)
{
    handle->cancel();
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
#include "gnunetpp-datastore.hpp"
#include <iostream>

using namespace gnunetpp;

DataStore::DataStore(const GNUNET_CONFIGURATION_Handle* cfg)
{
    datastore = GNUNET_DATASTORE_connect(cfg);
    registerService(this);
}

DataStore::~DataStore()
{
    shutdown();
    removeService(this);
}

void DataStore::shutdown()
{
    if(datastore != NULL)
    {
        GNUNET_DATASTORE_disconnect(datastore, GNUNET_NO);
        datastore = NULL;
    }
}

struct PutCallbackPack
{
    std::function<void(std::optional<std::string> error)> completedCallback;
};

struct GetCallbackPack
{
    std::function<void(std::optional<std::vector<uint8_t>>, uint64_t)> callback; 
};

static void put_callback (void *cls,
                int32_t success,
                struct GNUNET_TIME_Absolute min_expiration,
                const char *msg)
{
    auto pack = reinterpret_cast<PutCallbackPack*>(cls);
    std::optional<std::string> error;
    // Success == 0 means we have an error, else there should be no error message
    GNUNET_assert(!((msg == NULL) ^ (success == GNUNET_OK)));
    if(msg || !success)
        error = msg;
    pack->completedCallback(error);
    delete pack;
}

void get_callback(void *cls,
        const GNUNET_HashCode *key,
        size_t size,
        const void *data,
        GNUNET_BLOCK_Type type,
        uint32_t priority,
        uint32_t anonymity,
        uint32_t replication,
        GNUNET_TIME_Absolute expiration,
        uint64_t uid)
{
    auto pack = reinterpret_cast<GetCallbackPack*>(cls);
    std::optional<std::vector<uint8_t>> data_vec;
    GNUNET_assert((data == NULL && size == 0) || (data != NULL));
    if(data != NULL) {
        data_vec = std::vector<uint8_t>(size);
        memcpy(data_vec->data(), data, size);
    }
    pack->callback(data_vec, uid);
    delete pack;
}

void DataStore::put(const GNUNET_HashCode& key
    , const void* data, size_t data_size
    , std::function<void(std::optional<std::string> error)> completedCallback
    , std::chrono::seconds expiration
    , uint32_t priority
    , uint32_t replication
    , uint32_t anonymity
    , GNUNET_BLOCK_Type type
    , uint32_t queue_priority
    , uint32_t max_queue_size)
{
    size_t exp_msec = std::chrono::duration_cast<std::chrono::milliseconds>(expiration).count();
    GNUNET_TIME_Relative gnunet_expiration{exp_msec};
    GNUNET_TIME_Absolute gnunet_expiration_absolute = GNUNET_TIME_relative_to_absolute(gnunet_expiration);
    PutCallbackPack* pack = new PutCallbackPack{std::move(completedCallback)};
    auto handel = GNUNET_DATASTORE_put(datastore, 0, &key, data_size, data, type, priority, anonymity, replication
        , gnunet_expiration_absolute, queue_priority, max_queue_size, put_callback, pack);
    if(handel == NULL) {
        pack->completedCallback("GNUNET_DATASTORE_put failed");
        delete pack;
    }
}

cppcoro::task<> DataStore::put(const GNUNET_HashCode& key
    , const void* data, size_t data_size
    , std::chrono::seconds expiration
    , uint32_t priority
    , uint32_t replication
    , uint32_t anonymity
    , GNUNET_BLOCK_Type type
    , uint32_t queue_priority
    , uint32_t max_queue_size)
{
    struct PutAwaiter : public EagerAwaiter<>
    {
        PutAwaiter(DataStore* self
            , const GNUNET_HashCode& key
            , const void* data, size_t data_size
            , std::chrono::seconds expiration
            , uint32_t priority
            , uint32_t replication
            , uint32_t anonymity
            , GNUNET_BLOCK_Type type
            , uint32_t queue_priority
            , uint32_t max_queue_size)
    {
        self->put(key, data, data_size, [this](std::optional<std::string> error) {
            if(error)
                this->setException(std::make_exception_ptr(std::runtime_error(*error)));
            else
                this->setValue();
        }, expiration, priority, replication, anonymity, type, queue_priority, max_queue_size);
    }
    };
    PutAwaiter awaiter(this, key, data, data_size, expiration, priority, replication, anonymity, type, queue_priority, max_queue_size);
    co_await awaiter;
}

void DataStore::getOne(const GNUNET_HashCode& hash, std::function<void(std::optional<std::vector<uint8_t>>, uint64_t)> callback
    , uint32_t queue_priority, uint32_t max_queue_size, GNUNET_BLOCK_Type type, uint64_t uid)
{
    GetCallbackPack* pack = new GetCallbackPack{std::move(callback)};
    auto handel = GNUNET_DATASTORE_get_key(datastore, uid, GNUNET_NO, &hash, type, queue_priority, max_queue_size, get_callback, pack);
    if(handel == NULL) {
        pack->callback(std::nullopt, 0);
        delete pack;
    }
}

cppcoro::task<std::optional<std::vector<uint8_t>>> DataStore::getOne(const GNUNET_HashCode& hash
    , uint32_t queue_priority, uint32_t max_queue_size, GNUNET_BLOCK_Type type, uint64_t uid)
{
    struct GetAwaiter : public EagerAwaiter<std::optional<std::vector<uint8_t>>>
    {
        GetAwaiter(DataStore* self
            , const GNUNET_HashCode& hash
            , uint32_t queue_priority
            , uint32_t max_queue_size
            , GNUNET_BLOCK_Type type
            , uint64_t uid)
        {
            self->getOne(hash, [this](std::optional<std::vector<uint8_t>> data, uint64_t uid) {
                this->setValue(data);
            }, queue_priority, max_queue_size, type, uid);
        }
    };
    GetAwaiter awaiter(this, hash, queue_priority, max_queue_size, type, uid);
    co_return co_await awaiter;
}

cppcoro::async_generator<std::vector<uint8_t>> DataStore::get(const GNUNET_HashCode& hash
    , uint32_t queue_priority, uint32_t max_queue_size, GNUNET_BLOCK_Type type)
{
    struct GetAwaiter : public EagerAwaiter<std::pair<std::optional<std::vector<uint8_t>>, uint64_t>>
    {
        GetAwaiter(DataStore* self
            , const GNUNET_HashCode& hash
            , uint32_t queue_priority
            , uint32_t max_queue_size
            , GNUNET_BLOCK_Type type
            , uint64_t uid)
        {
            self->getOne(hash, [this](std::optional<std::vector<uint8_t>> data, uint64_t uid) {
                this->setValue({data, uid});
            }, queue_priority, max_queue_size, type, uid);
        }
    };
    uint64_t uid = 0;
    while(true) {
        GetAwaiter awaiter(this, hash, queue_priority, max_queue_size, type, uid);
        auto [data, new_uid] = co_await awaiter;
        if(!data)
            break;
        co_yield *data;
        uid = new_uid+1;
    }
}
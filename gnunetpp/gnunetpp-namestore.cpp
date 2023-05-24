#include "gnunetpp-namestore.hpp"

#include <iostream>

using namespace gnunetpp;

Namestore::Namestore(const GNUNET_CONFIGURATION_Handle* cfg)
{
    handle = GNUNET_NAMESTORE_connect(cfg);
    registerService(this);
}

Namestore::~Namestore()
{
    shutdown();
    removeService(this);
}

void Namestore::shutdown()
{
    if(handle != nullptr)
    {
        // Namestore seems to queue some work on shutdown, so we need delay
        // destruction of the handle until the service is stopped
        scheduler::queue([handle=this->handle](){
            GNUNET_NAMESTORE_disconnect(handle);
        });
        handle = nullptr;
    }
}

struct LookupCallbackPack
{
    std::function<void(std::vector<GNSRecord>)> cb;
};

static void lookup_callback(void *cls,
                            const GNUNET_IDENTITY_PrivateKey *zone,
                            const char *label,
                            unsigned int rd_count,
                            const GNUNET_GNSRECORD_Data *rd)
{
    GNUNET_assert(NULL != cls);
    auto pack = (LookupCallbackPack*)cls;
    std::vector<GNSRecord> records;
    if(rd_count != 0) {
        records.reserve(rd_count);
        for(unsigned int i = 0; i < rd_count; i++)
        {
            const auto& crd = rd[i];
            GNSRecord record;
            auto str = GNUNET_GNSRECORD_value_to_string(crd.record_type, crd.data, crd.data_size);

            if(str == nullptr) {
                std::cout << "Failed to convert record to string" << std::endl;
                continue;
            }
            auto type = GNUNET_GNSRECORD_number_to_typename(crd.record_type);
            if (type == nullptr) {
                std::cout << "Failed to convert record type to string" << std::endl;
                continue;
            }

            record.value = str;
            record.type = type;
            record.flags = crd.flags;

            GNUNET_free(str);

            bool relative_time = (int)crd.flags & GNUNET_GNSRECORD_RF_RELATIVE_EXPIRATION;
            if(relative_time)
                record.expiration_time_us = GNUNET_TIME_absolute_get().abs_value_us + crd.expiration_time;
            else
                record.expiration_time_us = crd.expiration_time;
            
            records.push_back(std::move(record));
        }
    }
    pack->cb(std::move(records));
    delete pack;
}

static void lookup_error_callback(void *cls)
{
    GNUNET_assert(NULL != cls);
    auto pack = (LookupCallbackPack*)cls;
    pack->cb(std::vector<GNSRecord>());
    delete pack;
}

void Namestore::lookup(const GNUNET_IDENTITY_PrivateKey& zone, const std::string& label, std::function<void(std::vector<GNSRecord>)> cb)
{
    auto pack = new LookupCallbackPack();
    pack->cb = std::move(cb);
    auto entry = GNUNET_NAMESTORE_records_lookup(handle, &zone, label.c_str(), lookup_error_callback, pack, lookup_callback, pack);
    if(entry == NULL) {
        delete pack;
        throw std::runtime_error("GNUNET_NAMESTORE_records_lookup returned NULL");
    }
}

struct StoreCallbackPack
{
    std::function<void(bool)> cb;
};

static void store_callback(void *cls, GNUNET_ErrorCode ec)
{
    GNUNET_assert(NULL != cls);
    auto pack = (StoreCallbackPack*)cls;
    pack->cb(ec == GNUNET_EC_NONE);
    delete pack;
}

void Namestore::store(const GNUNET_IDENTITY_PrivateKey& zone, const std::string& label, const std::vector<GNSRecord>& records, std::function<void(bool)> cb)
{
    GNUNET_assert(handle != nullptr);

    auto pack = new StoreCallbackPack();
    pack->cb = std::move(cb);

    // convert our records to the gnunet records
    std::vector<GNUNET_GNSRECORD_Data> gnunet_records;
    gnunet_records.reserve(records.size());
    for(const auto& record : records)
    {
        GNUNET_GNSRECORD_Data gnunet_record;
        auto type = GNUNET_GNSRECORD_typename_to_number(record.type.c_str());
        if(type == UINT32_MAX)
            throw std::runtime_error("Invalid record type " + record.type);
        void* data = nullptr;
        size_t data_size = 0;
        if(GNUNET_OK != GNUNET_GNSRECORD_string_to_value(type, record.value.c_str(), &data, &data_size))
            throw std::runtime_error("Failed to convert record value to data");
        gnunet_record.data = data;
        gnunet_record.data_size = data_size;
        gnunet_record.expiration_time = record.expiration_time_us;
        gnunet_record.flags = record.flags;
        gnunet_record.record_type = type;
        gnunet_records.push_back(std::move(gnunet_record));
    }

    auto entry = GNUNET_NAMESTORE_records_store(handle, &zone, label.c_str(), gnunet_records.size(), gnunet_records.data(), store_callback, pack);
    // release converted data
    for(auto& record : gnunet_records) {
        void* data = (void*)record.data;
        GNUNET_free(data);
    }
    if(entry == NULL) {
        delete pack;
        throw std::runtime_error("GNUNET_NAMESTORE_records_store returned NULL");
    }
}

cppcoro::task<std::vector<GNSRecord>> Namestore::lookup(GNUNET_IDENTITY_PrivateKey zone, const std::string label)
{
    struct LookupAwaiter : public EagerAwaiter<std::vector<GNSRecord>>
    {
        LookupAwaiter(Namestore* ns, const GNUNET_IDENTITY_PrivateKey& zone, const std::string& label)
        {
            ns->lookup(zone, label, [this](std::vector<GNSRecord> records) {
                this->setValue(std::move(records));
            });
        }
    };

    co_return co_await LookupAwaiter(this, zone, label);
}

cppcoro::task<bool> Namestore::store(GNUNET_IDENTITY_PrivateKey zone, const std::string label, const std::string value, std::string type
        , std::chrono::seconds experation, bool publish)
{
    struct StoreAwaiter : public EagerAwaiter<bool>
    {
        StoreAwaiter(Namestore* ns, const GNUNET_IDENTITY_PrivateKey& zone, const std::string& label, const std::string& value, std::string type
        , std::chrono::seconds experation, bool publish)
        {
            uint64_t usec = std::chrono::duration_cast<std::chrono::microseconds>(experation).count();
            std::vector<GNSRecord> records;
            GNSRecord record;
            record.value = value;
            record.type = type;
            record.expiration_time_us = usec;
            record.flags = (GNUNET_GNSRECORD_Flags)((publish ? 0 : GNUNET_GNSRECORD_RF_PRIVATE) | GNUNET_GNSRECORD_RF_RELATIVE_EXPIRATION);
            records.push_back(std::move(record));
            ns->store(zone, label, records, [this](bool success) {
                this->setValue(success);
            });
        }
    };

    co_return co_await StoreAwaiter(this, zone, label, value, type, experation, publish);
}

cppcoro::task<> Namestore::remove(GNUNET_IDENTITY_PrivateKey zone, const std::string label)
{
    struct RemoveAwaiter : public EagerAwaiter<>
    {
        RemoveAwaiter(Namestore* ns, const GNUNET_IDENTITY_PrivateKey& zone, const std::string& label)
        {
            ns->store(zone, label, {}, [this](bool success) {
                this->setValue();
            });
        }
    };

    co_await RemoveAwaiter(this, zone, label);
}
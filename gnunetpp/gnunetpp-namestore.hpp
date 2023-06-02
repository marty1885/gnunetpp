#pragma once

#include "inner/Infra.hpp"
#include "inner/coroutine.hpp"
#include "gnunetpp-crypto.hpp"

#include <gnunet/gnunet_namestore_service.h>

#include <chrono>

namespace gnunetpp
{

struct GNSRecord
{
    std::string value;
    std::string type;
    uint64_t expiration_time_us;
    GNUNET_GNSRECORD_Flags flags;
};

struct Namestore : public Service
{

    Namestore(const GNUNET_CONFIGURATION_Handle* cfg);
    void shutdown() override;
    virtual ~Namestore();

    /**
     * @brief Lookup records for a given label under a given zone (ego/identity)
     * 
     * @param zone the zone to lookup the label under
     * @param label the label to lookup
     * @param cb a callback that is called with the records
    */
    void lookup(const GNUNET_IDENTITY_PrivateKey& zone, const std::string& label, std::function<void(std::vector<GNSRecord>)> cb);
    cppcoro::task<std::vector<GNSRecord>> lookup(GNUNET_IDENTITY_PrivateKey zone, const std::string label);
    /**
     * @brief Store records for a given label under a given zone (ego/identity)
     * 
     * @param zone the zone to store the label under
     * @param label the label to store
     * @param records the records to store
     * @param cb a callback that is called with the result of the operation
     * 
     * @note This OVERWRITES any existing records for the given label. If you want to append records, use `lookup` and `store` instead.
    */
    void store(const GNUNET_IDENTITY_PrivateKey& zone, const std::string& label, const std::vector<GNSRecord>& records, std::function<void(bool)> cb);
    cppcoro::task<bool> store(GNUNET_IDENTITY_PrivateKey zone, const std::string label, const std::string value, std::string type
        , std::chrono::seconds experation = std::chrono::seconds(1200), bool publish = false);
    
    /**
     * @brief Remove records for a given label under a given zone (ego/identity)
     */
    cppcoro::task<> remove(GNUNET_IDENTITY_PrivateKey zone, const std::string label);

    GNUNET_NAMESTORE_Handle* handle = nullptr;
};
}
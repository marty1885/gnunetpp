#pragma once

#include "inner/Infra.hpp"
#include "inner/coroutine.hpp"
#include "gnunetpp-crypto.hpp"

#include <gnunet/gnunet_namestore_service.h>

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

    void lookup(const GNUNET_IDENTITY_PrivateKey& zone, const std::string& label, std::function<void(std::vector<GNSRecord>)> cb);
    void store(const GNUNET_IDENTITY_PrivateKey& zone, const std::string& label, const std::vector<GNSRecord>& records, std::function<void(bool)> cb);

    cppcoro::task<std::vector<GNSRecord>> lookup(GNUNET_IDENTITY_PrivateKey zone, const std::string label);
    cppcoro::task<bool> store(GNUNET_IDENTITY_PrivateKey zone, const std::string label, const std::string value, std::string type
        , std::chrono::seconds experation = std::chrono::seconds(1200), bool publish = false);
    cppcoro::task<> remove(GNUNET_IDENTITY_PrivateKey zone, const std::string label);

    GNUNET_NAMESTORE_Handle* handle = nullptr;
};
}
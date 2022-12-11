#pragma once

#include <string>
#include <chrono>

#include <gnunet/gnunet_gns_service.h>

#include "inner/Infra.hpp"
#include "inner/coroutine.hpp"

namespace gnunetpp
{

using GnsCallback = std::function<void(const std::vector<std::string> &)>;
using GnsErrorCallback = std::function<void(const std::string &)>;
struct GNS : public Service
{

    GNS(const GNUNET_CONFIGURATION_Handle *cfg);
    ~GNS()
    {
        shutdown();
        removeService(this);
    }
    void shutdown() override;

    /**
     * @brief Lookup a name in the GNS
     * 
     * @param name Domain name to lookup (e.g. "gnunet.org")
     * @param timeout Timeout for the lookup
     * @param cb Callback to call when the lookup is completed.
     * @param record_type The type of record to lookup. Defaults to ANY.
     * @param dns_compatability If true, the lookup will be done in a way that is compatible with DNS.
     */
    void lookup(const std::string &name, std::chrono::milliseconds timeout
                , GnsCallback cb
                , GnsErrorCallback err_cb
                , uint32_t record_type = GNUNET_GNSRECORD_TYPE_ANY
                , bool dns_compatability = true);
    void lookup(const std::string &name, std::chrono::milliseconds timeout
                , GnsCallback cb
                , GnsErrorCallback err_cb
                , const std::string_view record_type
                , bool dns_compatability = true);
    
    /**
     * @brief Lookup a name in the GNS. The coroutine version.
     * 
     * @param name Domain name to lookup (e.g. "gnunet.org")
     * @param timeout Timeout for the lookup
     * @param record_type The type of record to lookup. Defaults to ANY.
     * @return cppcoro::task<std::vector<std::string>> awiat to get the result
     */
    cppcoro::task<std::vector<std::string>> lookup(const std::string &name
        , std::chrono::milliseconds timeout
        , uint32_t record_type = GNUNET_GNSRECORD_TYPE_ANY
        , bool dns_compatability = true);
    cppcoro::task<std::vector<std::string>> lookup(const std::string &name
        , std::chrono::milliseconds timeout
        , const std::string_view record_type
        , bool dns_compatability = true);

    GNUNET_GNS_Handle *gns = nullptr;
};

}
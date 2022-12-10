#pragma once

#include <string>
#include <chrono>

#include <gnunet/gnunet_gns_service.h>

#include "inner/Infra.hpp"

namespace gnunetpp
{

using GnsCallback = std::function<void(const std::vector<std::string> &)>;
struct GNS : public Service
{

    GNS(const GNUNET_CONFIGURATION_Handle *cfg);

    void shutdown() override
    {
        if(gns != nullptr) {
            GNUNET_GNS_disconnect(gns);
            gns = nullptr;
        }
    }

    void lookup(const std::string &name, std::chrono::milliseconds timeout
                , GnsCallback cb
                , uint32_t record_type = GNUNET_GNSRECORD_TYPE_ANY
                , bool dns_compatability = true);
    
    void lookup(const std::string &name, std::chrono::milliseconds timeout
                , GnsCallback cb
                , const std::string_view record_type
                , bool dns_compatability = true);

    ~GNS()
    {
        shutdown();
        removeService(this);
    }

    GNUNET_GNS_Handle *gns = nullptr;
};

}
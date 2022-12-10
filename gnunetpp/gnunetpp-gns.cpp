#include "gnunetpp-gns.hpp"
#include "gnunetpp-scheduler.hpp"
#include "inner/UniqueData.hpp"

#include <gnunet/gnunet_util_lib.h>
#include <gnunet/gnunet_gnsrecord_lib.h>
#include <gnunet/gnunet_namestore_service.h>

#include <idna.h>

#include <iostream>

using namespace gnunetpp;

struct GnsCallbackPack
{
    GnsCallback cb;
    GNUNET_GNS_LookupWithTldRequest* lr = nullptr;
    TaskID id = 0;
    TaskID timeout_id = 0;
    uint32_t record_type = GNUNET_GNSRECORD_TYPE_ANY;
};

static detail::UniqueData<GnsCallbackPack*> gns_lookup_requests;
static void
process_lookup_result (void *cls,
                       int was_gns,
                       uint32_t rd_count,
                       const struct GNUNET_GNSRECORD_Data *rd)
{
    auto pack = static_cast<GnsCallbackPack*>(cls);
    if (rd_count == 0) {
        pack->cb({});
    }
    else {
        std::vector<std::string> results;
        for (unsigned int i = 0; i < rd_count; i++)
        {
            if(pack->record_type != GNUNET_GNSRECORD_TYPE_ANY && rd[i].record_type != pack->record_type)
                continue;
            char *rd_str = GNUNET_GNSRECORD_value_to_string (rd[i].record_type,
                                                    rd[i].data,
                                                    rd[i].data_size);
            results.push_back(rd_str);
            GNUNET_free (rd_str);
        }
        pack->cb(std::move(results));
    }

    gns_lookup_requests.remove(pack->id);
    scheduler::cancel(pack->timeout_id);
    delete pack;
}

GNS::GNS(const GNUNET_CONFIGURATION_Handle *cfg)
{
    gns = GNUNET_GNS_connect(cfg);
    if(gns == nullptr)
        throw std::runtime_error("Failed to connect to GNS service failed");
    registerService(this);
}

void GNS::lookup(const std::string &name, std::chrono::milliseconds timeout,
    GnsCallback cb, uint32_t record_type, bool dns_compatability)
{
    std::string lookup_name = name;
    if(gns == nullptr)
        throw std::runtime_error("GNS service not connected");
    if(dns_compatability && GNUNET_DNSPARSER_check_name(name.c_str()) == GNUNET_OK) {
        char* str = nullptr;
        if(idna_to_unicode_8z8z(name.c_str(), &str, 0) != IDNA_SUCCESS)
            throw std::runtime_error("Failed to convert name to unicode");
        lookup_name = str;
        free(str);
    }

    auto pack = new GnsCallbackPack;
    pack->cb = cb;
    pack->record_type = record_type;
    auto lr = GNUNET_GNS_lookup_with_tld(gns, lookup_name.c_str(), record_type, GNUNET_GNS_LO_DEFAULT, process_lookup_result, pack);
    pack->lr = lr;
    auto [id, _] = gns_lookup_requests.add((GnsCallbackPack*){pack});
    pack->id = id;
    auto timeout_id = scheduler::runLater(timeout, [pack]() {
        pack->cb({});
        GNUNET_GNS_lookup_with_tld_cancel(pack->lr);
        gns_lookup_requests.remove(pack->id);
        delete pack;
    }, true);
    pack->timeout_id = timeout_id;
}

void GNS::lookup(const std::string &name, std::chrono::milliseconds timeout,
    GnsCallback cb, const std::string_view record_type, bool dns_compatability)
{
    uint32_t type = GNUNET_GNSRECORD_typename_to_number(record_type.data());
    if(type == UINT32_MAX)
        throw std::runtime_error("Invalid record type");
    lookup(name, timeout, cb, type, dns_compatability);
}
#include "gnunetpp-gns.hpp"
#include "gnunetpp-scheduler.hpp"

#include <gnunet/gnunet_util_lib.h>
#include <gnunet/gnunet_gnsrecord_lib.h>
#include <gnunet/gnunet_namestore_service.h>

#include <idna.h>

using namespace gnunetpp;

struct GnsCallbackPack
{
    GnsCallback cb;
    GnsErrorCallback err_cb;
    GNUNET_GNS_LookupWithTldRequest* lr = nullptr;
    TaskID timeout_id = 0;
    uint32_t record_type = GNUNET_GNSRECORD_TYPE_ANY;
};

static void process_lookup_result (void *cls,
    int was_gns,
    uint32_t rd_count,
    const GNUNET_GNSRECORD_Data *rd)
{
    auto pack = reinterpret_cast<GnsCallbackPack*>(cls);
    if (rd_count == 0) {
        pack->cb({});
    }
    else {
        std::vector<std::pair<std::string, std::string>> results;
        results.reserve(rd_count);
        for (uint32_t i = 0; i < rd_count; i++) {
            if(pack->record_type != GNUNET_GNSRECORD_TYPE_ANY && rd[i].record_type != pack->record_type)
                continue;
            char *rd_str = GNUNET_GNSRECORD_value_to_string (rd[i].record_type,
                rd[i].data,
                rd[i].data_size);
            std::string rd_string = "<error>";
            if (rd_str != nullptr) {
                rd_string = rd_str;
                GNUNET_free (rd_str);
            }

            const char* type_str = GNUNET_GNSRECORD_number_to_typename(rd[i].record_type);
            std::string type_string = "<error>";
            if (type_str != nullptr)
                type_string = type_str;

            results.push_back({rd_string, type_string});
        }
        pack->cb(std::move(results));
    }

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

void GNS::shutdown()
{
    if(gns == nullptr)
        return;
    // In case we destruct while handling a lookup result
    scheduler::queue([gns=this->gns]() {
        GNUNET_GNS_disconnect(gns);
    });
    gns = nullptr;
}

void GNS::lookup(const std::string &name, std::chrono::milliseconds timeout,
    GnsCallback cb, GnsErrorCallback err_cb, uint32_t record_type, bool dns_compatability)
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
    auto lr = GNUNET_GNS_lookup_with_tld(gns, lookup_name.c_str(), record_type, GNUNET_GNS_LO_DEFAULT, process_lookup_result, pack);
    auto timeout_id = scheduler::runLater(timeout, [pack]() {
        pack->err_cb("Timeout");
        GNUNET_GNS_lookup_with_tld_cancel(pack->lr);
        delete pack;
    }, true);
    pack->cb = std::move(cb);
    pack->err_cb = std::move(err_cb);
    pack->record_type = record_type;
    pack->lr = lr;
    pack->timeout_id = timeout_id;
}

void GNS::lookup(const std::string &name, std::chrono::milliseconds timeout,
    GnsCallback cb, GnsErrorCallback err_cb, const std::string_view record_type, bool dns_compatability)
{
    uint32_t type = GNUNET_GNSRECORD_typename_to_number(record_type.data());
    if(type == UINT32_MAX)
        throw std::runtime_error("Invalid record type");
    lookup(name, timeout, std::move(cb), std::move(err_cb), type, dns_compatability);
}

cppcoro::task<std::vector<std::pair<std::string, std::string>>> GNS::lookup(const std::string &name, std::chrono::milliseconds timeout,
    uint32_t record_type, bool dns_compatability)
{
    struct RecordAwaiter : public EagerAwaiter<std::vector<std::pair<std::string, std::string>>>
    {
        RecordAwaiter(GNS& gns, const std::string& name, std::chrono::milliseconds timeout,
            uint32_t record_type, bool dns_compatability)
        {
            gns.lookup(name, timeout, [this](std::vector<std::pair<std::string, std::string>> results) {
                setValue(std::move(results));
            }, [this](const std::string& err){
                auto exception = std::make_exception_ptr(std::runtime_error(err));
                setException(exception);
            }, record_type, dns_compatability);
        }
    };
    co_return co_await RecordAwaiter(*this, name, timeout, record_type, dns_compatability);
}

cppcoro::task<std::vector<std::pair<std::string, std::string>>> GNS::lookup(const std::string &name, std::chrono::milliseconds timeout,
    const std::string_view record_type, bool dns_compatability)
{
    uint32_t type = GNUNET_GNSRECORD_typename_to_number(record_type.data());
    if(type == UINT32_MAX)
        throw std::runtime_error("Invalid record type");
    co_return co_await lookup(name, timeout, type, dns_compatability);
}
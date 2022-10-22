#include "Infra.hpp"

#include <mutex>
#include <set>
#include <cassert>
#include <memory>

#include <gnunetpp-scheduler.hpp>

namespace gnunetpp
{
static std::mutex g_mtx_services;
static std::set<Service*> g_services;

void registerService(Service* service)
{
    std::lock_guard<std::mutex> m(g_mtx_services);
    g_services.insert(service);
}

void removeService(Service* service)
{
    std::lock_guard<std::mutex> m(g_mtx_services);
    service->shutdown();
    g_services.erase(service);
}

void removeAllServices()
{
    std::lock_guard<std::mutex> m(g_mtx_services);
    for (auto& service : g_services)
        service->shutdown();
    g_services.clear();
}

void run(std::function<void(const GNUNET_CONFIGURATION_Handle*)> f)
{
    using CallbackType = std::function<void(const GNUNET_CONFIGURATION_Handle* c)>;

    const char* args_dummy = "gnunetpp";
    CallbackType* functor = new CallbackType(std::move(f));
    struct GNUNET_GETOPT_CommandLineOption options[] = {
        GNUNET_GETOPT_OPTION_END
    };
    auto r = GNUNET_PROGRAM_run(1, const_cast<char**>(&args_dummy), "gnunetpp", "no help", options
        , [](void *cls, char *const *args, const char *cfgfile
            , const GNUNET_CONFIGURATION_Handle* c) {
                std::unique_ptr<CallbackType> functor{static_cast<CallbackType*>(cls)};
                assert(functor != nullptr);
                (*functor)(c);
            }
        , functor);
    if(r != GNUNET_OK)
        throw std::runtime_error("GNUNet program run failed");
}

void shutdown()
{
    GNUNET_SCHEDULER_add_now([] (void* d) {
        removeAllServices();
        scheduler::shutdown();
    }, NULL);
}
}
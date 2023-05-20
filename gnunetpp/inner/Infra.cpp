#include "Infra.hpp"

#include <mutex>
#include <set>
#include <cassert>
#include <memory>
#include <random>
#include <thread>

#include <gnunetpp-scheduler.hpp>

static int g_notify_fds[2] = {-1, -1};

namespace gnunetpp
{
namespace detail
{
thread_local std::mt19937_64 g_rng(std::random_device{}());
static std::thread::id g_scheduler_thread_id;
void notifyWakeup()
{
    if(g_notify_fds[1] != -1)
        write(g_notify_fds[1], "1", 1);
}

static void installNotifyFds()
{
    GNUNET_assert(g_notify_fds[0] == -1 && g_notify_fds[1] == -1);
    GNUNET_assert(pipe(g_notify_fds) == 0);

    auto rs = GNUNET_NETWORK_fdset_create();
    GNUNET_NETWORK_fdset_set_native(rs, g_notify_fds[0]);
    auto task = GNUNET_SCHEDULER_add_select(GNUNET_SCHEDULER_PRIORITY_URGENT, GNUNET_TIME_UNIT_FOREVER_REL, rs, nullptr
    , [] (void* cls) {
        char buf[1024];
        while(true) {
            ssize_t len = read(g_notify_fds[0], buf, sizeof(buf));
            GNUNET_assert(len >= 0);
            // if there's less data in the pipe then our buffer, we're done reading everything
            if(len != sizeof(buf))
                break;
        }
    }, nullptr);
    GNUNET_SCHEDULER_add_shutdown([] (void* cls) {
        auto task = reinterpret_cast<GNUNET_SCHEDULER_Task*>(cls);
        GNUNET_SCHEDULER_cancel(task);
        close(g_notify_fds[0]);
        close(g_notify_fds[1]);
        g_notify_fds[0] = -1;
        g_notify_fds[1] = -1;
    }, reinterpret_cast<void*>(task));
    GNUNET_NETWORK_fdset_destroy(rs);
}

}
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

static bool g_running = false;
void run(std::function<void(const GNUNET_CONFIGURATION_Handle*)> f, const std::string& service_name)
{
    using CallbackType = std::function<void(const GNUNET_CONFIGURATION_Handle* c)>;

    const char* args_dummy = "gnunetpp";
    CallbackType* functor = new CallbackType(std::move(f));
    struct GNUNET_GETOPT_CommandLineOption options[] = {
        GNUNET_GETOPT_OPTION_END
    };
    auto r = GNUNET_PROGRAM_run(1, const_cast<char**>(&args_dummy), service_name.c_str(), "no help", options
        , [](void *cls, char *const *args, const char *cfgfile
            , const GNUNET_CONFIGURATION_Handle* c) {
                detail::installNotifyFds();
                detail::g_scheduler_thread_id = std::this_thread::get_id();
                std::unique_ptr<CallbackType> functor{static_cast<CallbackType*>(cls)};
                assert(functor != nullptr);

                g_running = true;
                // Needed to make GNUNet react to Ctrl+C
                scheduler::runOnShutdown([]{
                    shutdown();
                });
                (*functor)(c);
            }
        , functor);
    if(r != GNUNET_OK)
        throw std::runtime_error("GNUNet program run failed");
}

void start(std::function<cppcoro::task<void>(const GNUNET_CONFIGURATION_Handle*)> f, const std::string& service_name)
{
    run([f = std::move(f)](const GNUNET_CONFIGURATION_Handle* c) {
        async_run([f = std::move(f), c]() -> cppcoro::task<> {
            co_await f(c);
        });
    });
}

void shutdown()
{
    if(!g_running)
        return;
    GNUNET_SCHEDULER_add_now([] (void* d) {
        scheduler::cancelAll();
        removeAllServices();
        scheduler::shutdown();
    }, NULL);
    if(!inMainThread())
        detail::notifyWakeup();
}

bool inMainThread()
{
    return detail::g_scheduler_thread_id == std::this_thread::get_id();
}
}
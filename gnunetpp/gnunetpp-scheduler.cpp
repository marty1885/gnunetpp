#include "gnunetpp-scheduler.hpp"
#include "gnunet/platform.h"
#include <gnunet/gnunet_core_service.h>

#include "inner/UniqueData.hpp"

#include <map>
#include <cassert>
#include <mutex>
#include <random>

namespace gnunetpp::scheduler
{

struct TaskData
{
    GNUNET_SCHEDULER_Task* handle;
    std::function<void()> fn;
};

static detail::UniqueData<TaskData> g_tasks;
TaskID runLater(std::chrono::duration<double> delay, std::function<void()> fn)
{
    using namespace std::chrono;
    const uint64_t usec = duration_cast<microseconds>(delay).count();
    GNUNET_TIME_Relative time{usec};

    auto [id, data] = g_tasks.add({nullptr, std::move(fn)});
    static_assert(sizeof(TaskID) == sizeof(void*));
    auto handle = GNUNET_SCHEDULER_add_delayed(time, [] (void *cls) {
        size_t id = reinterpret_cast<size_t>(cls);
        g_tasks[id].fn();
        g_tasks.remove(id);
    }, reinterpret_cast<void*>(id));
    data.handle = handle;
    return id;
}

void runOnShutdown(std::function<void()> fn)
{
    GNUNET_SCHEDULER_add_shutdown([] (void *cls) {
        auto fn = reinterpret_cast<std::function<void()>*>(cls);
        (*fn)();
        delete fn;
    }, new std::function<void()>(std::move(fn)));
}

void run(std::function<void()> fn)
{
    GNUNET_SCHEDULER_run([] (void *cls) {
        auto fn = reinterpret_cast<std::function<void()>*>(cls);
        (*fn)();
        delete fn;
    }, new std::function<void()>(std::move(fn)));
}

void cancel(TaskID id)
{
    auto& data = g_tasks[id];
    GNUNET_SCHEDULER_cancel(data.handle);
    g_tasks.remove(id);
}

void shutdown()
{
    GNUNET_SCHEDULER_shutdown();
}

}
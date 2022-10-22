#include "gnunetpp-scheduler.hpp"
#include "gnunet/platform.h"
#include <gnunet/gnunet_core_service.h>

#include <map>
#include <cassert>
#include <mutex>
#include <random>

namespace gnunetpp::scheduler
{

struct TaskData
{
    TaskID id;
    GNUNET_SCHEDULER_Task* handle;
    std::function<void()> fn;
};

static std::map<TaskID, TaskData> g_tasks;
static std::mutex g_mtx_tasks;
TaskID runLater(std::chrono::duration<double> delay, std::function<void()> fn)
{
    using namespace std::chrono;
    const uint64_t usec = duration_cast<microseconds>(delay).count();
    GNUNET_TIME_Relative time{usec};

    std::lock_guard lock(g_mtx_tasks);
    size_t id = 0;
    thread_local std::mt19937_64 gen{std::random_device{}()};
    do {
        id = gen();
    } while (g_tasks.find(id) != g_tasks.end());

    auto handle = GNUNET_SCHEDULER_add_delayed(time, [] (void *cls) {
        auto it = [cls]() {
            std::lock_guard lock(g_mtx_tasks);
            return g_tasks.find(reinterpret_cast<TaskID>(cls));
        }();
        assert(it != g_tasks.end());
        auto& task = it->second;
        task.fn();
        {
            std::lock_guard lock(g_mtx_tasks);
            g_tasks.erase(it);
        }
    }, reinterpret_cast<void*>(id));

    g_tasks[id] = {id, handle, fn};
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
    std::lock_guard lock(g_mtx_tasks);
    auto it = g_tasks.find(id);
#ifndef NDEBUG
    if(it == g_tasks.end())
        return;
#endif
    assert(it != g_tasks.end());
    auto& task = it->second;
    GNUNET_SCHEDULER_cancel(task.handle);
    g_tasks.erase(it);
}

void shutdown()
{
    GNUNET_SCHEDULER_shutdown();
}

}
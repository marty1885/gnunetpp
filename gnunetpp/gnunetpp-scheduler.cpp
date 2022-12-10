#include "gnunetpp-scheduler.hpp"
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
    bool repeat = false;
};

static detail::UniqueData<TaskData> g_tasks;
static void timer_callback_trampoline(void* cls)
{
    auto start = std::chrono::steady_clock::now();
    size_t id = reinterpret_cast<size_t>(cls);
    auto& data = g_tasks[id];
    data.fn();
    auto end = std::chrono::steady_clock::now();
    if(data.repeat) {
        auto diff = end - start;
        auto next_trigger_time = std::chrono::duration_cast<std::chrono::milliseconds>(diff);
        GNUNET_TIME_Relative delay{(uint64_t)next_trigger_time.count()};
        data.handle = GNUNET_SCHEDULER_add_delayed(delay, &timer_callback_trampoline, cls);
    }
    else
        g_tasks.remove(id);
}

static TaskID runDelay(std::chrono::duration<double> delay, std::function<void()> fn, bool repeat)
{
    using namespace std::chrono;
    const uint64_t usec = duration_cast<microseconds>(delay).count();
    GNUNET_TIME_Relative time{usec};

    auto [id, data] = g_tasks.add({nullptr, std::move(fn), repeat});
    static_assert(sizeof(TaskID) == sizeof(void*));
    auto handle = GNUNET_SCHEDULER_add_delayed(time,timer_callback_trampoline , reinterpret_cast<void*>(id));
    data.handle = handle;
    return id;
}

TaskID runLater(std::chrono::duration<double> delay, std::function<void()> fn)
{
    return runDelay(delay, std::move(fn), false);
}

TaskID runEvery(std::chrono::duration<double> delay, std::function<void()> fn)
{
    return runDelay(delay, std::move(fn), true);
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
    GNUNET_SCHEDULER_add_now([] (void *cls) {
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

void cancelAll()
{
    for(auto& [id, data] : g_tasks)
        GNUNET_SCHEDULER_cancel(data.handle);
    g_tasks.clear();
}

static bool running = true;
void shutdown()
{
    if(running) {
        cancelAll();
        GNUNET_SCHEDULER_shutdown();
    }
    running = false;
}

}

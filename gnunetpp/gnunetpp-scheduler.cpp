#include "gnunetpp-scheduler.hpp"
#include <gnunet/gnunet_core_service.h>

#include "inner/Infra.hpp"
#include "inner/UniqueData.hpp"

#include <map>
#include <cassert>
#include <mutex>
#include <random>

#include <iostream>

namespace gnunetpp::scheduler
{

struct TaskData
{
    GNUNET_SCHEDULER_Task* handle;
    std::function<void()> fn;
    bool repeat = false;
    bool run_on_shutdown = false;
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

static TaskID runDelay(std::chrono::duration<double> delay, std::function<void()> fn, bool repeat, bool run_on_shutdown)
{
    using namespace std::chrono;
    const uint64_t usec = duration_cast<microseconds>(delay).count();
    GNUNET_TIME_Relative time{usec};

    auto [id, data] = g_tasks.add({nullptr, std::move(fn), repeat, run_on_shutdown});
    static_assert(sizeof(TaskID) == sizeof(void*));
    auto handle = GNUNET_SCHEDULER_add_delayed(time,timer_callback_trampoline , reinterpret_cast<void*>(id));
    data.handle = handle;
    wakeUp();
    return id;
}

TaskID runLater(std::chrono::duration<double> delay, std::function<void()> fn, bool run_on_shutdown)
{
    return runDelay(delay, std::move(fn), false, run_on_shutdown);
}

TaskID runEvery(std::chrono::duration<double> delay, std::function<void()> fn)
{
    return runDelay(delay, std::move(fn), true, false);
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
    wakeUp();
}

cppcoro::task<> sleep(std::chrono::duration<double> delay)
{
    struct TimerAwaiter : public CallbackAwaiter<>
    {
        TimerAwaiter(std::chrono::duration<double> delay) : delay_(delay) {}

        void await_suspend(std::coroutine_handle<> handle)
        {
            runLater(delay_, [handle] () mutable {
                handle.resume();
            }, false);
        }
        std::chrono::duration<double> delay_;
    };
    co_await TimerAwaiter(delay);
}

void cancel(TaskID id)
{
    auto& data = g_tasks[id];
    GNUNET_SCHEDULER_cancel(data.handle);
    g_tasks.remove(id);
}

void cancelAll()
{
    for(auto& [id, data] : g_tasks) {
        if(data.run_on_shutdown)
            data.fn();
        GNUNET_SCHEDULER_cancel(data.handle);
    }
    g_tasks.clear();
}

static bool running = true;
void shutdown()
{
    if(running)
        GNUNET_SCHEDULER_shutdown();
    running = false;
}

struct ReadLineCallbackPack
{
    std::function<void(const std::string&)> fn;
    GNUNET_SCHEDULER_Task* task;
    GNUNET_SCHEDULER_Task* shutdown_task;
};

void readStdin(std::function<void(const std::string&)> fn)
{
    auto rs = GNUNET_NETWORK_fdset_create();
    GNUNET_NETWORK_fdset_set_native(rs, 0);

    auto pack = new ReadLineCallbackPack{std::move(fn), nullptr, nullptr};
    pack->shutdown_task = GNUNET_SCHEDULER_add_shutdown([] (void* cls) {
        auto pack = reinterpret_cast<ReadLineCallbackPack*>(cls);
        GNUNET_SCHEDULER_cancel(pack->task);
        delete pack;
    }, pack);

    auto task = GNUNET_SCHEDULER_add_select(GNUNET_SCHEDULER_PRIORITY_DEFAULT, GNUNET_TIME_UNIT_FOREVER_REL, rs, nullptr
    , [] (void* cls) {
        auto pack = reinterpret_cast<ReadLineCallbackPack*>(cls);
        GNUNET_SCHEDULER_cancel(pack->shutdown_task);
        char* line = nullptr;
        size_t len = 0;
        ssize_t read = getline(&line, &len, stdin);
        if(read == -1) {
            delete pack;
            return;
        }
        std::string str(line, read);
        pack->fn(str);
        delete pack;
    }, pack);
    pack->task = task;
    GNUNET_NETWORK_fdset_destroy(rs);
    wakeUp();
}

cppcoro::task<std::string> readStdin()
{
    struct ReadLineAwaiter : public CallbackAwaiter<std::string>
    {
        void await_suspend(std::coroutine_handle<> handle)
        {
            gnunetpp::scheduler::readStdin([handle, this] (const std::string& line) {
                setValue(line);
                handle.resume();
            });
        }
    };
    co_return co_await ReadLineAwaiter();
}

cppcoro::task<> waitUntilShutdown()
{
    struct ShutdownAwaiter : public CallbackAwaiter<>
    {
        void await_suspend(std::coroutine_handle<> handle)
        {
            GNUNET_SCHEDULER_add_shutdown([] (void* cls) {
                auto handle = reinterpret_cast<std::coroutine_handle<>*>(cls);
                (*handle).resume();
            }, new std::coroutine_handle<>(handle));
        }
    };
    co_await ShutdownAwaiter();
}

void wakeUp()
{
    if(!inMainThread())
        detail::notifyWakeup();
}

}

#pragma once

#include <functional>
#include <chrono>

#include "inner/coroutine.hpp"

namespace gnunetpp
{
using TaskID = size_t;

// HACK: I don't want to include inner/Infa.hpp here, for better compile times
bool inMainThread();
}

namespace gnunetpp::scheduler
{
/**
 * @brief Run a function after a delay
 * 
 * @param delay delay in seconds
 * @param fn function to run
 * @param run_on_shutdown if true, the function will run at shutdown even if it hasn't timed out yet
 * @return TaskID Handle to the task
 */
TaskID runLater(std::chrono::microseconds delay, std::function<void()> fn, bool run_on_shutdown = false);

/**
 * @brief Run a function every `delay` seconds
 * 
 * @param delay delay in seconds
 * @param fn function to run
 * @return TaskID Handle to the task
 */
TaskID runEvery(std::chrono::microseconds delay, std::function<void()> fn);

/**
 * @brief Register a function to run on shutdown
 * 
 * @param fn function to run
 */
void runOnShutdown(std::function<void()> fn);

/**
 * @brief Run a function immediately after the main thread enters the scheduler
 * 
 * @param fn function to run
 */
void queue(std::function<void()> fn);

/**
 * @brief Run a function immediately if the current thread is the scheduler thread, otherwise queue it
 * 
 * @param fn function to run
 */
template<typename Fn>
void run(Fn&& fn)
{
    if (inMainThread())
        fn();
    else
        queue(std::forward<Fn>(fn));
}

/**
 * @brief Resumes execution after a delay
 * 
 * @param delay delay in seconds
 * @return cppcoro::task<> await on this to resume execution after the delay
 */
[[nodiscard]]
cppcoro::task<> sleep(std::chrono::microseconds delay);

/**
 * @brief Cancel a task
 * 
 * @param id the task ID to cancel
 */
void cancel(TaskID id);

/**
 * @brief Cancel all tasks
 * 
 */
void cancelAll();

/**
 * @brief Shutdown the scheduler
 */
void shutdown();

/**
 * @brief Asynchronously read a line from stdin
 */
void readStdin(std::function<void(const std::string&, bool)> fn);
cppcoro::task<std::string> readStdin();

/**
 * @brief Asynchronously wait until shutdown
 * 
 */
cppcoro::task<> waitUntilShutdown();

/**
 * @brief Force the GNUnet scheduler to run
 * 
 */
void wakeUp();

/**
 * @brief Run on GNUnet main thread
*/
cppcoro::task<> runOnMainThread();
}
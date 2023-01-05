#pragma once

#include <functional>
#include <chrono>

#include "inner/coroutine.hpp"

namespace gnunetpp
{
using TaskID = size_t;
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
TaskID runLater(std::chrono::duration<double> delay, std::function<void()> fn, bool run_on_shutdown = false);

/**
 * @brief Run a function every `delay` seconds
 * 
 * @param delay delay in seconds
 * @param fn function to run
 * @return TaskID Handle to the task
 */
TaskID runEvery(std::chrono::duration<double> delay, std::function<void()> fn);

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
void run(std::function<void()> fn);

/**
 * @brief Resumes execution after a delay
 * 
 * @param delay delay in seconds
 * @return cppcoro::task<> await on this to resume execution after the delay
 */
[[nodiscard]]
cppcoro::task<> sleep(std::chrono::duration<double> delay);

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
void readStdin(std::function<void(const std::string&)> fn);
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
}
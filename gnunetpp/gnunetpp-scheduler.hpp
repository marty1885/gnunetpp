#pragma once

#include <functional>
#include <chrono>

namespace gnunetpp
{
using TaskID = size_t;
}

namespace gnunetpp::scheduler
{
TaskID runLater(std::chrono::duration<double> delay, std::function<void()> fn, bool run_on_shutdown = false);
TaskID runEvery(std::chrono::duration<double> delay, std::function<void()> fn);
void runOnShutdown(std::function<void()> fn);
void run(std::function<void()> fn);
void cancel(TaskID id);
void cancelAll();
void shutdown();
}
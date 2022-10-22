#pragma once

#include <functional>
#include <chrono>

namespace gnunetpp::scheduler
{
using TaskID = size_t;
TaskID runLater(std::chrono::duration<double> delay, std::function<void()> fn);
void runOnShutdown(std::function<void()> fn);
void run(std::function<void()> fn);
void cancel(TaskID id);
void shutdown();
}
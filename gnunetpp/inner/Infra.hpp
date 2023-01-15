#pragma once

#include <functional>

#include <gnunet/gnunet_core_service.h>
#include "coroutine.hpp"
#include "NonCopyable.hpp"

namespace gnunetpp
{
namespace detail
{
/**
 * @brief Wakes the GNUnet scheduler up if it is sleeping
 * 
 */
void notifyWakeup();
}

struct Service : public NonCopyable
{
    virtual ~Service() = default;

    virtual void shutdown() = 0;
protected:
};

void registerService(Service* service);
void removeService(Service* service);
void removeAllServices();

/**
 * @brief Starts GNUnet and runs the event loop
 * 
 * @param f Callback to call when GNUnet is started
 */
void run(std::function<void(const GNUNET_CONFIGURATION_Handle*)> f, const std::string& service_name = "gnunetpp");

/**
 * @brief run but the coroutine version
 * 
 * @param f Callback to call when GNUnet is started
 */
void start(std::function<cppcoro::task<>(const GNUNET_CONFIGURATION_Handle*)> f, const std::string& service_name = "gnunetpp");

/**
 * @brief Shutdown all services and timer. Stop the event loop
 */
void shutdown();

/**
 * @brief Returns true the current execting thread is the same thread running GNUnet
 */
bool inMainThread();
}
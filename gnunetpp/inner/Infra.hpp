#pragma once

#include <functional>

#include <gnunet/gnunet_core_service.h>
#include "coroutine.hpp"

namespace gnunetpp
{

struct Service
{
    virtual ~Service() = default;
    Service(const Service&) = delete;
    Service& operator=(const Service&) = delete;
    Service(Service&&) = delete;
    Service& operator=(Service&&) = delete;

    virtual void shutdown() = 0;
protected:
    Service() = default;
};

void registerService(Service* service);
void removeService(Service* service);
void removeAllServices();

/**
 * @brief Starts GNUnet and runs the event loop
 * 
 * @param f Callback to call when GNUnet is started
 */
void run(std::function<void(const GNUNET_CONFIGURATION_Handle*)> f);

/**
 * @brief run but the coroutine version
 * 
 * @param f Callback to call when GNUnet is started
 */
void start(std::function<cppcoro::task<>(const GNUNET_CONFIGURATION_Handle*)> f);

/**
 * @brief Shutdown all services and timer. Stop the event loop
 */
void shutdown();

}
#pragma once

#include <functional>

#include <gnunet/platform.h>
#include <gnunet/gnunet_core_service.h>

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
void run(std::function<void(const GNUNET_CONFIGURATION_Handle*)> f);
void shutdown();

}
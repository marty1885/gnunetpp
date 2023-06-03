#include "gnunetpp-nse.hpp"
#include <cmath>

#include <iostream>

using namespace gnunetpp;

struct EstimateAwaiter : public CallbackAwaiter<>
{
    EstimateAwaiter(NSE* nse) : nse(nse) {}

    void await_suspend(std::coroutine_handle<> handle)
    {
        if (nse->estimate_)
            handle.resume();
        else {
            nse->observers_single_shot_.push_back([handle] (auto) {
                handle.resume();
            });
        }
    }
    NSE* nse = nullptr;
};

static void handle_estimate(void *cls,
    GNUNET_TIME_Absolute timestamp,
    double estimate,
    double std_dev)
{
    auto nse = static_cast<NSE*>(cls);
    nse->estimate_ = {GNUNET_NSE_log_estimate_to_n(estimate), std_dev};

    for(auto& observer : nse->observers_)
        observer(*nse->estimate_);
    for(auto& observer : nse->observers_single_shot_)
        observer(*nse->estimate_);
    nse->observers_single_shot_.clear();
}

NSE::NSE(const GNUNET_CONFIGURATION_Handle *cfg)
{
    nse = GNUNET_NSE_connect(cfg, &handle_estimate, this);
    registerService(this);
}

NSE::~NSE()
{
    shutdown();
    removeService(this);
}

void NSE::shutdown()
{
    if (nse)
        GNUNET_NSE_disconnect(nse);
    nse = nullptr;
}

Task<std::pair<double, double>> NSE::estimate()
{
    co_await EstimateAwaiter{this};
    co_return *estimate_;
}
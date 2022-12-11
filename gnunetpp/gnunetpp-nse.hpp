#pragma once

#include <gnunet/gnunet_nse_service.h>
#include <gnunet/gnunet_util_lib.h>

#include <inner/Infra.hpp>
#include <inner/coroutine.hpp>

#include <optional>
#include <utility>
#include <functional>
#include <vector>

namespace gnunetpp {
struct NSE : public Service
{
    NSE(const GNUNET_CONFIGURATION_Handle *cfg);
    ~NSE();
    void shutdown() override;

    /**
     * @brief  Get the GNUnet network size estimate
    */
    [[nodiscard]]
    cppcoro::task<std::pair<double, double>> estimate();

    /**
     * @brief Watch for estimate changes
     * 
     * @param callback Gets called when the estimate changes
     */
    void watch(std::function<void(std::pair<double, double>)> callback);

    GNUNET_NSE_Handle* nse = nullptr;
    std::optional<std::pair<double, double>> estimate_;
    std::vector<std::function<void(std::pair<double, double>)>> observers_;
    std::vector<std::function<void(std::pair<double, double>)>> observers_single_shot_;
};
}
#pragma once

#include <gnunet/platform.h>
#include <gnunet/gnunet_fs_service.h>

#include "inner/UniqueData.hpp"
#include "inner/Infra.hpp"

#include "gnunetpp-scheduler.hpp"

#include <vector>
#include <string>
#include <functional>
#include <chrono>
#include <memory>

namespace gnunetpp::FS
{
using FSCallbackFunctor = std::function<void(const GNUNET_FS_ProgressInfo *)>;
namespace detail
{
struct FSCallbackData
{
    FSCallbackFunctor fn;
    GNUNET_FS_Handle* fs;
    GNUNET_FS_SearchContext* sc;
    GNUNET_FS_DownloadContext* dc;
    TaskID timeout_task;
};
extern std::map<GNUNET_FS_Handle*, detail::FSCallbackData*> g_fs_handlers;
GNUNET_FS_Handle* makeHandle(const GNUNET_CONFIGURATION_Handle* cfg, FSCallbackFunctor callback);
void* fs_callback_trampoline(void *cls, const struct GNUNET_FS_ProgressInfo *info);

}

GNUNET_FS_SearchContext* search(
    const GNUNET_CONFIGURATION_Handle* cfg,
    const std::vector<std::string>& keywords,
    std::function<bool(const std::string_view, const std::string_view)> fn,
    std::chrono::duration<double> timeout = std::chrono::seconds(30),
    GNUNET_FS_SearchOptions options = GNUNET_FS_SEARCH_OPTION_NONE,
    unsigned anonymity_level = 1);

enum class DownloadStatus
{
    Started,
    Error,
    Completed,
    Progress,
    Cancelled
};

GNUNET_FS_DownloadContext* download(
    const GNUNET_CONFIGURATION_Handle* cfg,
    const std::string& uri,
    const std::string& filename,
    std::function<void(DownloadStatus)> fn,
    unsigned anonymity_level = 1);

void cancel(GNUNET_FS_SearchContext* sc);
}
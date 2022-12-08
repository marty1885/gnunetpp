#pragma once

#include <gnunet/gnunet_fs_service.h>

#include "inner/UniqueData.hpp"
#include "inner/Infra.hpp"

#include "gnunetpp-scheduler.hpp"
#include "gnunetpp-identity.hpp"

#include <vector>
#include <string>
#include <functional>
#include <chrono>
#include <memory>

namespace gnunetpp::FS
{
enum class PublishResult
{
    Success,
    Error
};
using FSCallbackFunctor = std::function<void(const GNUNET_FS_ProgressInfo *)>;
using ScanCallbackFunctor = std::function<void(GNUNET_FS_DirScanner*, const std::string&, bool, GNUNET_FS_DirScannerProgressUpdateReason)>;
using PublishCallbackFunctor = std::function<void(PublishResult, const std::string&, const std::string&)>;
using UnindexCallbackFunctor = std::function<void(bool, const std::string&)>;
namespace detail
{
struct FSCallbackData
{
    FSCallbackFunctor fn;
    GNUNET_FS_Handle* fs;
    GNUNET_FS_SearchContext* sc;
    GNUNET_FS_DownloadContext* dc;
    GNUNET_FS_UnindexContext* uc;
    TaskID timeout_task;
};
struct ScanCallbackData
{
    ScanCallbackFunctor fn;
    GNUNET_FS_DirScanner* ds;
};
extern std::map<GNUNET_FS_Handle*, detail::FSCallbackData*> g_fs_handlers;
GNUNET_FS_Handle* makeHandle(const GNUNET_CONFIGURATION_Handle* cfg, FSCallbackFunctor callback);
GNUNET_FS_Handle* makeHandle(const GNUNET_CONFIGURATION_Handle* cfg, FSCallbackFunctor callback, unsigned download_parallelism, unsigned request_parallelism);
void* fs_callback_trampoline(void *cls, const struct GNUNET_FS_ProgressInfo *info);
static GNUNET_FS_BlockOptions default_block_options = { { 0LL }, 1, 365, 1 };
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

using DownloadCallbackFunctor = std::function<void(DownloadStatus, const std::string&, size_t, size_t)>;

GNUNET_FS_DownloadContext* download(
    const GNUNET_CONFIGURATION_Handle* cfg,
    const std::string& uri,
    const std::string& filename,
    DownloadCallbackFunctor fn,
    unsigned anonymity_level = 1,
    unsigned download_parallelism = 16,
    unsigned request_parallelism = 4092);

void publish(
    const GNUNET_CONFIGURATION_Handle* cfg,
    const std::string& filename,
    const std::vector<std::string>& keywords,
    PublishCallbackFunctor fn,
    std::chrono::seconds experation = std::chrono::seconds(3600*24*365),
    GNUNET_IDENTITY_Ego* ego = nullptr, 
    const std::string& this_id = "",
    const std::string& next_id = "",
    GNUNET_FS_BlockOptions block_options = detail::default_block_options);

GNUNET_FS_DirScanner* scan(
    const GNUNET_CONFIGURATION_Handle* cfg,
    const std::string& filename,
    ScanCallbackFunctor fn);
GNUNET_FS_UnindexContext* unindex(
    const GNUNET_CONFIGURATION_Handle* cfg,
    const std::string& file,
    UnindexCallbackFunctor fn);

void cancel(GNUNET_FS_SearchContext* sc);
}

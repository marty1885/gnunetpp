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

/**
 * @brief Searches the File Share for files matching the given keywords.
 * 
 * @param cfg handle to GNUnet
 * @param keywords the keywords to search for
 * @param fn callback function that is called for each result
 * @param timeout how long to wait for results
 * @param options search options
 * @param anonymity_level the anonymity level to use for the search (> 1 uses GAP for anonymity)
 * @return GNUNET_FS_SearchContext* handle to the operation. Can be used to cancel the search.
 */
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

/**
 * @brief Downloads a file from the File Share.
 * 
 * @param cfg handle to GNUnet
 * @param uri KSK or CHK the URI of the file to download
 * @param filename the filename to save the file to
 * @param fn callback function that is called on download progress
 * @param anonymity_level the anonymity level to use for the download (> 1 uses GAP for anonymity)
 * @param download_parallelism Number of parallel downloads to use
 * @param request_parallelism Number of requests to send in parallel
 * @return GNUNET_FS_DownloadContext* handle to the operation. Can be used to cancel the download.
 */
GNUNET_FS_DownloadContext* download(
    const GNUNET_CONFIGURATION_Handle* cfg,
    const std::string& uri,
    const std::string& filename,
    DownloadCallbackFunctor fn,
    unsigned anonymity_level = 1,
    unsigned download_parallelism = 16,
    unsigned request_parallelism = 4092);

/**
 * @brief Publishes a file to the File Share.
 * 
 * @param cfg handle to GNUnet
 * @param filename path to the file to publish
 * @param keywords additional keywords to publish with the file
 * @param fn callback function that is called when the operation is complete
 * @param experation how long the file should be published for (may be ignored by peers)
 * @param ego the ego to publish the file with (nullptr for none)
 * @param this_id the last file ID (for updating files. must provide an ego)
 * @param next_id the expected next file ID (for updating files. must provide an ego)
 * @param block_options options for the file blocks. Experation is overriden by the experation parameter.
 */
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

// Should this be considered an internal function?
GNUNET_FS_DirScanner* scan(
    const GNUNET_CONFIGURATION_Handle* cfg,
    const std::string& filename,
    ScanCallbackFunctor fn);

/**
 * @brief Unindexes a file from the local File Share.
 * @note This DOES NOT remove the file from the network as other nodes may have it.
 * 
 * @param cfg handle to GNUnet
 * @param file the file to unindex (path to local file)
 * @param fn callback function that is called when the operation is complete
 * @return GNUNET_FS_UnindexContext* handle to the operation. Can be used to cancel the unindexing.
 */
GNUNET_FS_UnindexContext* unindex(
    const GNUNET_CONFIGURATION_Handle* cfg,
    const std::string& file,
    UnindexCallbackFunctor fn);

cppcoro::task<> unindex(
    const GNUNET_CONFIGURATION_Handle* cfg,
    const std::string& file);

/**
 * @brief Cancels a search operation.
 * 
 * @param sc handle to the search operation
 */
void cancel(GNUNET_FS_SearchContext* sc);
}

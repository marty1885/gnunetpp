#include "gnunetpp-fs.hpp"

#include <stdexcept>
#include <iostream>

namespace gnunetpp::FS
{
namespace detail
{
std::map<GNUNET_FS_Handle*, detail::FSCallbackData*> g_fs_handlers;
void* fs_callback_trampoline(void *cls, const struct GNUNET_FS_ProgressInfo *info)
{
    // HCAK: 
    if(info->status == GNUNET_FS_STATUS_DOWNLOAD_SUSPEND)
        return nullptr;
    auto pack = reinterpret_cast<detail::FSCallbackData*>(cls);
    pack->fn(info);
    return nullptr;
}

GNUNET_FS_Handle* makeHandle(const GNUNET_CONFIGURATION_Handle* cfg, FSCallbackFunctor callback)
{
    FSCallbackData* data = new FSCallbackData;
    data->fn = callback;
    GNUNET_FS_Handle* fs_handle = GNUNET_FS_start(cfg, "gnunetpp-fs", &fs_callback_trampoline
        , data, GNUNET_FS_FLAGS_NONE, GNUNET_FS_OPTIONS_END);
    data->fs = fs_handle;
    g_fs_handlers[fs_handle] = data;
    if (!fs_handle)
        throw std::runtime_error("GNUNET_FS_start failed");
    return fs_handle;
}

GNUNET_FS_Handle* makeHandle(const GNUNET_CONFIGURATION_Handle* cfg, FSCallbackFunctor callback, unsigned download_parallelism, unsigned request_parallelism)
{
    FSCallbackData* data = new FSCallbackData;
    data->fn = callback;
    GNUNET_FS_Handle* fs_handle = GNUNET_FS_start(cfg, "gnunetpp-fs", &fs_callback_trampoline
        , data, GNUNET_FS_FLAGS_NONE, GNUNET_FS_OPTIONS_DOWNLOAD_PARALLELISM, download_parallelism
        , GNUNET_FS_OPTIONS_REQUEST_PARALLELISM, request_parallelism
        , GNUNET_FS_OPTIONS_END);
    data->fs = fs_handle;
    g_fs_handlers[fs_handle] = data;
    if (!fs_handle)
        throw std::runtime_error("GNUNET_FS_start failed");
    return fs_handle;
}

}

GNUNET_FS_SearchContext* search(
    const GNUNET_CONFIGURATION_Handle* cfg,
    const std::vector<std::string>& keywords,
    std::function<bool(const std::string_view, const std::string_view)> fn,
    std::chrono::duration<double> timeout,
    GNUNET_FS_SearchOptions options,
    unsigned anonymity_level)
{
    auto fs_handle = detail::makeHandle(cfg, [fn=std::move(fn)](const GNUNET_FS_ProgressInfo * info){
        if(info->status == GNUNET_FS_STATUS_SEARCH_STOPPED) {
            scheduler::run([fsh=info->fsh](){
                auto it = detail::g_fs_handlers.find(fsh);
                assert(it != detail::g_fs_handlers.end());
                assert(it->second->fs = fsh);

                auto timeout_task = it->second->timeout_task;
                if(timeout_task != 0)
                    scheduler::cancel(it->second->timeout_task);
                delete it->second;
                detail::g_fs_handlers.erase(it);
                GNUNET_FS_stop(fsh);
            });
            return;
        }
        else if(info->status == GNUNET_FS_STATUS_SEARCH_ERROR) {
            std::cerr << "GNUNet++: FS  Search Error: " << info->value.search.specifics.error.message << std::endl;
            auto it = detail::g_fs_handlers.find(info->fsh);
            assert(it != detail::g_fs_handlers.end());
            assert(it->second->fs = info->fsh);
            scheduler::run([sc = it->second->sc](){
                GNUNET_FS_search_stop(sc);
            });
        }
        else if(info->status != GNUNET_FS_STATUS_SEARCH_RESULT)
            return;
        char* uri = GNUNET_FS_uri_to_string(info->value.search.specifics.result.uri);
        char* filename = GNUNET_CONTAINER_meta_data_get_by_type (
            info->value.search.specifics.result.meta,
            (EXTRACTOR_MetaType)EXTRACTOR_METATYPE_GNUNET_ORIGINAL_FILENAME);
        bool keep_running = fn(uri, filename);
        GNUNET_free_nz(filename);
        GNUNET_free_nz(uri);
        if(keep_running == false) {
            auto it = detail::g_fs_handlers.find(info->fsh);
            assert(it != detail::g_fs_handlers.end());
            assert(it->second->fs = info->fsh);
            scheduler::run([sc = it->second->sc](){
                GNUNET_FS_search_stop(sc);
            });
        }
    });
    std::vector<const char*> keywords_cstr;
    for(const auto& keyword : keywords)
        keywords_cstr.push_back(keyword.c_str());
    GNUNET_FS_Uri* uri = GNUNET_FS_uri_ksk_create_from_args(keywords_cstr.size(), keywords_cstr.data());
    if(uri == NULL)
        throw std::runtime_error("Failed to create KSK URI");

    GNUNET_FS_SearchContext* search = GNUNET_FS_search_start(fs_handle, uri, anonymity_level, options, NULL);
    if(search == NULL) {
        detail::g_fs_handlers.erase(fs_handle);
        GNUNET_FS_stop(fs_handle);
        GNUNET_FS_uri_destroy(uri);
        throw std::runtime_error("Failed to start search");
    }
    auto pack = detail::g_fs_handlers[fs_handle];
    pack->sc = search;

    TaskID timeout_task = scheduler::runLater(timeout, [fs_handle, search, pack](){
        pack->timeout_task = 0;
        GNUNET_FS_search_stop(search);
    });
    pack->timeout_task = timeout_task;

    GNUNET_FS_uri_destroy(uri);
    return search;
}

void cancel(GNUNET_FS_SearchContext* search)
{
    scheduler::run([search](){
        GNUNET_FS_search_stop(search);
    });
}

GNUNET_FS_DownloadContext* download(
    const GNUNET_CONFIGURATION_Handle* cfg,
    const std::string& uri,
    const std::string& filename,
    std::function<void(DownloadStatus)> fn,
    unsigned anonymity_level,
    unsigned download_parallelism,
    unsigned request_parallelism)
{
    char** error_msg = NULL;
    auto gnet_uri = GNUNET_FS_uri_parse(uri.c_str(), error_msg);
    if(gnet_uri == NULL)
        throw std::runtime_error("Failed to parse URI");
    if(GNUNET_FS_uri_test_chk(gnet_uri) != GNUNET_OK && GNUNET_FS_uri_test_loc(gnet_uri) != GNUNET_OK) {
        GNUNET_FS_uri_destroy(gnet_uri);
        throw std::runtime_error("URI must be CHK (shared files) or LOC (file on specific node)");
    }

    auto fs_handle = detail::makeHandle(cfg, [fn=std::move(fn)](const GNUNET_FS_ProgressInfo * info){
        auto safe_cleanup = [fsh=info->fsh](){
                auto it = detail::g_fs_handlers.find(fsh);
                assert(it != detail::g_fs_handlers.end());
                assert(it->second->fs = fsh);

                auto timeout_task = it->second->timeout_task;
                // if(timeout_task != 0)
                //     scheduler::cancel(it->second->timeout_task);
                delete it->second;
                detail::g_fs_handlers.erase(it);
                GNUNET_FS_stop(fsh);
            };
        if(info->status == GNUNET_FS_STATUS_DOWNLOAD_STOPPED) {
            fn(DownloadStatus::Cancelled);
            scheduler::run(safe_cleanup);
            return;
        }
        else if(info->status == GNUNET_FS_STATUS_DOWNLOAD_ERROR) {
            fn(DownloadStatus::Error);
            scheduler::run(safe_cleanup);
        }
        else if(info->status == GNUNET_FS_STATUS_DOWNLOAD_PROGRESS) {
            fn(DownloadStatus::Progress);
        }
        else if(info->status == GNUNET_FS_STATUS_DOWNLOAD_COMPLETED) {
            fn(DownloadStatus::Completed);
            scheduler::run(safe_cleanup);
        }
        else if(info->status == GNUNET_FS_STATUS_DOWNLOAD_START) {
            fn(DownloadStatus::Started);
        }
    }, download_parallelism, request_parallelism);

    // TODO: allow changing options
    GNUNET_FS_DownloadOptions options = GNUNET_FS_DOWNLOAD_OPTION_RECURSIVE;
    GNUNET_FS_DownloadContext* download = GNUNET_FS_download_start(fs_handle, gnet_uri, NULL, filename.c_str(), NULL, 0
        , GNUNET_FS_uri_chk_get_file_size(gnet_uri), anonymity_level, options, NULL, NULL);
    detail::g_fs_handlers[fs_handle]->dc = download;
    GNUNET_FS_uri_destroy(gnet_uri);
    return download;
}
}
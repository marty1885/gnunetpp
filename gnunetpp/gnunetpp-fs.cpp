#include "gnunetpp-fs.hpp"
#include "gnunetpp-identity.hpp"

#include <stdexcept>
#include <iostream>

namespace gnunetpp::FS
{
namespace detail
{
struct InspectData
{
    std::vector<std::string> keywords;
    GNUNET_FS_FileInformation* fi = nullptr;
};
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

static void directory_scan_trampoline(void *cls,
    const char *filename,
    int is_directory,
    enum GNUNET_FS_DirScannerProgressUpdateReason reason)
{
    auto pack = reinterpret_cast<detail::ScanCallbackData*>(cls);
    std::string name;
    if(filename)
        name = filename;
    pack->fn(pack->ds, name, is_directory, reason);

    if(reason == GNUNET_FS_DIRSCANNER_FINISHED || reason == GNUNET_FS_DIRSCANNER_INTERNAL_ERROR)
        delete pack;
}

/**
 * YANKED FROM GNUNET DIRECTLY
 * Iterate over the results from the directory scan and extract
 * the desired information for the publishing operation.
 *
 * @param item root with the data from the directory scan
 * @return handle with the information for the publishing operation
 */
static struct GNUNET_FS_FileInformation *
get_file_information (GNUNET_FS_Handle* fsh, GNUNET_FS_ShareTreeItem *item, const GNUNET_FS_BlockOptions& bo, bool insert = false)
{
  struct GNUNET_FS_FileInformation *fi;
  struct GNUNET_FS_FileInformation *fic;
  struct GNUNET_FS_ShareTreeItem *child;

  if (GNUNET_YES == item->is_directory)
  {
    if (NULL == item->meta)
      item->meta = GNUNET_CONTAINER_meta_data_create ();
    GNUNET_CONTAINER_meta_data_delete (item->meta,
                                       EXTRACTOR_METATYPE_MIMETYPE,
                                       NULL,
                                       0);
    GNUNET_FS_meta_data_make_directory (item->meta);
    if (NULL == item->ksk_uri)
    {
      const char *mime = GNUNET_FS_DIRECTORY_MIME;
      item->ksk_uri = GNUNET_FS_uri_ksk_create_from_args (1, &mime);
    }
    else
      GNUNET_FS_uri_ksk_add_keyword (item->ksk_uri,
                                     GNUNET_FS_DIRECTORY_MIME,
                                     GNUNET_NO);
    fi = GNUNET_FS_file_information_create_empty_directory (fsh,
                                                            NULL,
                                                            item->ksk_uri,
                                                            item->meta,
                                                            &bo,
                                                            item->filename);
    for (child = item->children_head; child; child = child->next)
    {
      fic = get_file_information(fsh, child, bo, insert);
      GNUNET_break (GNUNET_OK == GNUNET_FS_file_information_add (fi, fic));
    }
  }
  else
  {
    fi = GNUNET_FS_file_information_create_from_file (fsh,
                                                      NULL,
                                                      item->filename,
                                                      item->ksk_uri,
                                                      item->meta,
                                                      ! insert,
                                                      &bo);
  }
  return fi;
}

static int
publish_inspector (void *cls,
                   struct GNUNET_FS_FileInformation *fi,
                   uint64_t length,
                   struct GNUNET_CONTAINER_MetaData *m,
                   struct GNUNET_FS_Uri **uri,
                   struct GNUNET_FS_BlockOptions *bo,
                   int *do_index,
                   void **client_info)
{
  char *fn;
  char *fs;
  struct GNUNET_FS_Uri *new_uri;

  auto data = reinterpret_cast<detail::InspectData*>(cls);
  if (data->fi == fi)
    return GNUNET_OK;
  if(data->keywords.size() > 0)
  {
    std::vector<const char*> keywords;
    for(auto& kw : data->keywords)
        keywords.push_back(kw.c_str());
    auto keyword_uri = GNUNET_FS_uri_ksk_create_from_args(keywords.size(), keywords.data());
    if(*uri == NULL)
        *uri = keyword_uri;
    else {
        auto new_uri = GNUNET_FS_uri_ksk_merge(keyword_uri, *uri);
        GNUNET_FS_uri_destroy(*uri);
        *uri = new_uri;
        GNUNET_FS_uri_destroy(keyword_uri);
    }
    data->keywords.clear();
  }
  if (/*enable_creation_time*/ true)
    GNUNET_CONTAINER_meta_data_add_publication_date (m);
  if (GNUNET_YES == GNUNET_FS_meta_data_test_for_directory(m)) {
    data->fi = fi;
    GNUNET_FS_file_information_inspect(fi, &publish_inspector, data);
  }
  return GNUNET_OK;
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
                auto pack = it->second;
                assert(pack->fs = fsh);

                auto timeout_task = pack->timeout_task;
                // if(timeout_task != 0)
                //     scheduler::cancel(it->second->timeout_task);
                detail::g_fs_handlers.erase(it);
                GNUNET_FS_stop(fsh);
                delete pack;
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

void publish(
    const GNUNET_CONFIGURATION_Handle* cfg,
    const std::string& filename,
    const std::vector<std::string>& keywords,
    std::function<void(const std::string&)> fn,
    GNUNET_IDENTITY_Ego* ego, 
    const std::string& this_id,
    const std::string& next_id,
    GNUNET_FS_BlockOptions block_options)
{
    if(next_id.empty() ^ this_id.empty())
        throw std::runtime_error("Must specify both this_id and next_id or neither");
    
    auto fs_handle = detail::makeHandle(cfg, [cb=std::move(fn)](const GNUNET_FS_ProgressInfo * info){
        if(info->status == GNUNET_FS_STATUS_PUBLISH_COMPLETED) {
            auto it = detail::g_fs_handlers.find(info->fsh);
            assert(it != detail::g_fs_handlers.end());
            assert(it->second->fs = info->fsh);
            auto pack = it->second;
            auto fn = pack->fn;
            auto uri = GNUNET_FS_uri_to_string(info->value.publish.specifics.completed.chk_uri);
            assert(uri != NULL);
            cb(uri);
            GNUNET_free_nz(uri);

            if(info->value.publish.pctx == NULL) {
                detail::g_fs_handlers.erase(it);
                    scheduler::run([fsh=info->fsh, pack](){
                    GNUNET_FS_stop(fsh);
                    delete pack;
                });
            }
        }
        else if(info->status == GNUNET_FS_STATUS_PUBLISH_ERROR)
        {
            auto it = detail::g_fs_handlers.find(info->fsh);
            assert(it != detail::g_fs_handlers.end());
            assert(it->second->fs = info->fsh);
            auto pack = it->second;
            auto fn = pack->fn;
            cb(info->value.publish.specifics.error.message);
            detail::g_fs_handlers.erase(it);
            scheduler::run([fsh=info->fsh, pack](){
                GNUNET_FS_stop(fsh);
                delete pack;
            });
        }
    });
    if(fs_handle == NULL)
        throw std::runtime_error("Failed to connect to FS service");
    
    scan(cfg, filename, [fs_handle, this_id, next_id, block_options, ego, keywords](GNUNET_FS_DirScanner* ds, const std::string& filename, bool is_dir, GNUNET_FS_DirScannerProgressUpdateReason reason){
        if(reason == GNUNET_FS_DIRSCANNER_FINISHED) {
            auto directory_scan_result = GNUNET_FS_directory_scan_get_result(ds);
            GNUNET_FS_share_tree_trim(directory_scan_result);
            auto fi = detail::get_file_information(fs_handle, directory_scan_result, block_options, false);
            GNUNET_FS_share_tree_free(directory_scan_result);
            if(fi == NULL)
                // TODO: Need a better way to handle this
                throw std::runtime_error("Failed to get file information");
            auto insp_data = new detail::InspectData;
            // FIXME: Keywords seems to not be working
            insp_data->keywords = std::move(keywords);
            GNUNET_FS_file_information_inspect(fi, &detail::publish_inspector, insp_data);
            delete insp_data;

            const struct GNUNET_CRYPTO_EcdsaPrivateKey *priv = NULL;
            if(ego != NULL) {
                auto sk = identity::get_private_key(ego);
                if(sk->type != GNUNET_IDENTITY_TYPE_ECDSA)
                    throw std::runtime_error("Only ECDSA keys are supported");
                priv = &sk->ecdsa_key;
            }
            auto pc = GNUNET_FS_publish_start (fs_handle,
                                fi,
                                priv,
                                this_id.c_str(),
                                next_id.c_str(),
                                GNUNET_FS_PUBLISH_OPTION_NONE);
            if(pc == NULL)
                throw std::runtime_error("Failed to start publish");
        }
    });
}

GNUNET_FS_DirScanner* scan(
    const GNUNET_CONFIGURATION_Handle* cfg,
    const std::string& filename,
    ScanCallbackFunctor fn)
{
    if(access(filename.c_str(), R_OK) != 0)
        throw std::runtime_error("Cannot access " + filename);

    char* ex = NULL;
    if(GNUNET_CONFIGURATION_get_value_string(cfg, "fs", "EXTRACTORS", &ex) != GNUNET_OK)
        ex = NULL;
    
    auto data = new detail::ScanCallbackData;
    data->fn = std::move(fn);
    auto ds = GNUNET_FS_directory_scan_start(filename.c_str(), false, ex, &detail::directory_scan_trampoline, data);
    if(ds == NULL) {
        GNUNET_free(ex);
        delete data;
        throw std::runtime_error("Failed to start meta directory scanner. Is gnunet-helper-publish-fs installed?");
    }
    data->ds = ds;
    GNUNET_free(ex);
    return ds;
}

}
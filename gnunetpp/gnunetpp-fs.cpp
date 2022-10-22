#include "gnunetpp-fs.hpp"

#include <stdexcept>
#include <iostream>

namespace gnunetpp
{

FS::FS(const GNUNET_CONFIGURATION_Handle* cfg)
{
    fs_handle = GNUNET_FS_start(cfg, "gnunetpp-fs", &FS::progress_callback, (void*)this, GNUNET_FS_FLAGS_NONE, GNUNET_FS_OPTIONS_END);
    if(fs_handle == NULL)
        throw std::runtime_error("Failed to start GNUNet FS");
    registerService(this);
}

GNUNET_FS_SearchContext* FS::search(const std::vector<std::string>& keywords
    , std::function<bool(const std::string_view)> callback
    , std::chrono::duration<double> search_timeout
    , GNUNET_FS_SearchOptions options
    , unsigned int anonymity_level)
{
    if(fs_handle == NULL)
        throw std::runtime_error("FS not connected");
    std::vector<const char*> keywords_cstr;
    for(const auto& keyword : keywords)
        keywords_cstr.push_back(keyword.c_str());
    GNUNET_FS_Uri* uri = GNUNET_FS_uri_ksk_create_from_args(keywords_cstr.size(), keywords_cstr.data());
    if(uri == NULL)
        throw std::runtime_error("Failed to create KSK URI");

    auto data = new CallbackData{callback, this, OpType::Search};
    GNUNET_FS_SearchContext* search = GNUNET_FS_search_start(fs_handle, uri, anonymity_level, options, NULL);
    if(search == NULL) {
        delete data;
        GNUNET_FS_uri_destroy(uri);
        throw std::runtime_error("Failed to start search");
    }
    ongoing_operations[search] = data;

    GNUNET_FS_uri_destroy(uri);
    return search;
}

void* FS::progress_callback_real(const struct GNUNET_FS_ProgressInfo *info)
{
    
    if(info->status == GNUNET_FS_STATUS_SEARCH_RESULT)
    {
        char* uri = GNUNET_FS_uri_to_string(info->value.search.specifics.result.uri);
        char* filename = GNUNET_CONTAINER_meta_data_get_by_type (
            info->value.search.specifics.result.meta,
            (EXTRACTOR_MetaType)EXTRACTOR_METATYPE_GNUNET_ORIGINAL_FILENAME);
        std::cout << "Found `" << filename << "` at " << uri << "!" << std::endl;
    }
    return 0;
}

void* FS::progress_callback(void *cls, const struct GNUNET_FS_ProgressInfo *info)
{
    return ((FS*)cls)->progress_callback_real(info);
}

}
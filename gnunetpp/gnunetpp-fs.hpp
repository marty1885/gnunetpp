#pragma once

#include <gnunet/platform.h>
#include <gnunet/gnunet_fs_service.h>

#include "inner/UniqueData.hpp"
#include "inner/Infra.hpp"

#include <vector>
#include <string>
#include <functional>
#include <chrono>

namespace gnunetpp
{
struct FS : public Service
{
    enum class OpType
    {
        Search,
        Download
    };
    struct CallbackData
    {
        std::function<bool(const std::string_view)> callback;
        FS* self;
        OpType type;
    };
    FS(const GNUNET_CONFIGURATION_Handle* cfg);
    ~FS()
    {
        shutdown();
        removeService(this);
    }

    void shutdown() override
    {
        if (fs_handle)
        {
            GNUNET_FS_stop(fs_handle);
            fs_handle = nullptr;
        }
    }

    GNUNET_FS_SearchContext* search(const std::vector<std::string>& keywords
        , std::function<bool(const std::string_view)> callback
        , std::chrono::duration<double> search_timeout = std::chrono::seconds(30)
        , GNUNET_FS_SearchOptions options = GNUNET_FS_SEARCH_OPTION_NONE
        , unsigned int anonymity_level = 1);

    GNUNET_FS_Handle* native_handle() const
    {
        return fs_handle;
    }
    
protected:
    GNUNET_FS_Handle* fs_handle = NULL;
    std::map<GNUNET_FS_SearchContext*, CallbackData*> ongoing_operations;

    static void* progress_callback(void *cls, const struct GNUNET_FS_ProgressInfo *info);
    void* progress_callback_real(const struct GNUNET_FS_ProgressInfo *info);
};
}
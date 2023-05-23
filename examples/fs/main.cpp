#include "CLI/App.hpp"
#include "CLI/Formatter.hpp"
#include "CLI/Config.hpp"

#include <gnunetpp.hpp>
#include <iostream>
#include <iomanip>

std::string uri;
std::string output_name;
std::vector<std::string> keywords;
std::string filename;
size_t timeout = 10;
size_t max_results = 0;
uint32_t anonymity_level = 1;

bool run_download;
bool run_search;
bool run_publish;
bool run_unindex;


cppcoro::task<> service(const GNUNET_CONFIGURATION_Handle* cfg)
{
    if(run_search) {
        gnunetpp::FS::search(cfg, keywords, [n=0](const std::string_view uri, const std::string_view original_file_name) mutable -> bool {
            // the last number at the end of the uri is the file size (according to the spec). We can extract it and print it.
            auto pos = uri.find_last_of('.');
            assert(pos != std::string_view::npos);
            auto size = std::stoull(std::string(uri.substr(pos+1)));
            std::stringstream ss;
            ss << std::fixed << std::setprecision(2);
            if(size > 1000*1000)
                ss << size / (float)(1000*1000) << " MB";
            else if(size > 1000)
                ss << size / (float)1000 << " KB";
            else
                ss << size << " B";

            std::cout << "Found " << original_file_name << ". size " << ss.str() << std::endl
                << "gnunetpp-fs download -o '" << original_file_name << "' " << uri << std::endl
                << std::endl;
            return max_results == 0 || ++n < max_results;
        }, std::chrono::seconds(timeout), GNUNET_FS_SEARCH_OPTION_NONE, anonymity_level);
    }

    else if(run_download) {
        gnunetpp::FS::download(cfg, uri, output_name
            , [](gnunetpp::FS::DownloadStatus status, const std::string& message, size_t downloaded, size_t total) {
                if(status == gnunetpp::FS::DownloadStatus::Progress) {
                    double percent = (double)downloaded / (double)total * 100.0;
                    std::cout << "\33[2K\rDownloaded " << downloaded << " of " << total << " bytes (" << percent << "%)" << std::flush;
                }
                else if(status == gnunetpp::FS::DownloadStatus::Completed) {
                    std::cout << "\nDone." << std::endl;
                    gnunetpp::shutdown();
                }
                else if(status == gnunetpp::FS::DownloadStatus::Started) {
                    std::cout << "Starting to download!" << std::endl;
                }
        }, anonymity_level);
    }

    else if(run_publish) {
        try {
            auto [uri, namespace_uri] = co_await gnunetpp::FS::publish(cfg, filename, keywords);
            std::cout << "Published " << filename << " at " << uri << std::endl;
            if(namespace_uri.empty() == false)
                std::cout << "Namespace URI: " << (namespace_uri.empty() ? "(none)" : namespace_uri.c_str()) << std::endl;
        }
        catch(const std::exception& e) {
            std::cout << "Publish failed" << std::endl;
        }
    }

    else if(run_unindex) {
        // Unindex removes the file from the index. But it does not un-publish it. Un-publishing can't be done as
        // other nodes might still have the file.
        try {
            co_await gnunetpp::FS::unindex(cfg, filename);
            std::cout << "Unindexed " << filename << std::endl;
        }
        catch(const std::exception& e) {
            std::cerr << "Unindex failed: " << e.what() << std::endl;
        }
    }
}

int main(int argc, char** argv)
{
    CLI::App app("GNUnet++ program to interact with the File Sharing GNUnet system", "gnunetpp-fs");
    app.require_subcommand(1);

    auto download = app.add_subcommand("download", "Download a file from GNUnet File Share");
    download->add_option("uri", uri, "URI of the file to download")->required();
    download->add_option("-o,--output", output_name, "Output file name")->required();
    download->add_option("-a,--anonymity", anonymity_level, "Anonymity level [0, inf]. "
        "0 is none; 1 with GAP; >1 with cover traffic")->default_val(uint32_t{1});

    auto search = app.add_subcommand("search", "Search GNUnet File Share using keywords");
    search->add_option("keywords", keywords, "Keywords to search for")->required();
    search->add_option("-t,--timeout", timeout, "Timeout in seconds")->default_val(size_t{10});
    search->add_option("-n,--max-results", max_results, "Maximum number of results to return (0 means unlimited)")->default_val(size_t{0});
    search->add_option("-a,--anonymity", anonymity_level, "Anonymity level [0, inf]. "
        "0 is none; 1 with GAP; >1 with cover traffic")->default_val(uint32_t{1});

    auto publish = app.add_subcommand("publish", "Publish file/directory to GNUnet File Share");
    publish->add_option("file", filename, "File or directory to publish")->required();
    publish->add_option("-k,--keyword", keywords, "Keywords to publish with");

    auto unindex = app.add_subcommand("unindex", "Unindex file/directory from local index");
    unindex->add_option("file", filename, "File or directory to unindex")->required();

    CLI11_PARSE(app, argc, argv);

    run_download = download->parsed();
    run_search = search->parsed();
    run_publish = publish->parsed();
    run_unindex = unindex->parsed();

    gnunetpp::start(service);
}
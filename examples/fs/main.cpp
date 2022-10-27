#include "CLI/App.hpp"
#include "CLI/Formatter.hpp"
#include "CLI/Config.hpp"

#include <gnunetpp.hpp>
#include <iostream>

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


void service(const GNUNET_CONFIGURATION_Handle* cfg)
{
    if(run_search) {
        gnunetpp::FS::search(cfg, keywords, [n=0](const std::string_view uri, const std::string_view original_file_name) mutable -> bool {
            std::cout << "Found " << original_file_name << std::endl
                << "gnunetpp-fs download -o '" << original_file_name << "' " << uri << std::endl
                << std::endl;
            return ++n < max_results;
        }, std::chrono::seconds(timeout), GNUNET_FS_SEARCH_OPTION_NONE, anonymity_level);
    }

    else if(run_download) {
        gnunetpp::FS::download(cfg, uri, output_name
            , [](gnunetpp::FS::DownloadStatus status) {
                // TODO: Make this look nicer
                std::cout << "Download status: " << static_cast<int>(status) << std::endl;
                if(status == gnunetpp::FS::DownloadStatus::Completed)
                    gnunetpp::shutdown();
        }, anonymity_level);
    }

    else if(run_publish) {
        gnunetpp::FS::publish(cfg, filename, keywords
            , [](gnunetpp::FS::PublishResult status, const std::string& uri, const std::string& namespace_uri) {
            if(status == gnunetpp::FS::PublishResult::Success) {
                std::cout << "Published " << filename << " at " << uri << std::endl;
                if(namespace_uri.size() > 0)
                    std::cout << "Namespace URI: " << (namespace_uri.empty() ? "(none)" : namespace_uri.c_str()) << std::endl;
            }
            else {
                std::cout << "Publish failed" << std::endl;
            }
        });
    }
}

int main(int argc, char** argv)
{
    CLI::App app{"gnunetpp-fs"};
    app.require_subcommand(1);

    auto download = app.add_subcommand("download", "Download a file from GNUnet");
    download->add_option("uri", uri, "URI of the file to download")->required();
    download->add_option("-o,--output", output_name, "Output file name")->required();
    download->add_option("-a,--anonymity", anonymity_level, "Anonymity level [0, inf]. "
        "0 is none; 1 with GAP; >1 with cover traffic")->default_val(uint32_t{1});

    auto search = app.add_subcommand("search", "Search files with keywords");
    search->add_option("keywords", keywords, "Keywords to search for")->required();
    search->add_option("-t,--timeout", timeout, "Timeout in seconds")->default_val(size_t{10});
    search->add_option("-n,--max-results", max_results, "Maximum number of results to return (0 means unlimited)")->default_val(size_t{0});
    search->add_option("-a,--anonymity", anonymity_level, "Anonymity level [0, inf]. "
        "0 is none; 1 with GAP; >1 with cover traffic")->default_val(uint32_t{1});

    auto publish = app.add_subcommand("publish", "Publish file/directory to GNUnet");
    publish->add_option("file", filename, "File or directory to publish")->required();
    publish->add_option("-k,--keyword", keywords, "Keywords to publish with");

    CLI11_PARSE(app, argc, argv);

    run_download = download->parsed();
    run_search = search->parsed();
    run_publish = publish->parsed();

    gnunetpp::run(service);
}
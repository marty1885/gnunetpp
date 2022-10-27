#include <gnunetpp.hpp>
#include <iostream>

void service(const GNUNET_CONFIGURATION_Handle* cfg)
{
    // gnunetpp::FS::search(cfg, {"txt"}, [](const std::string_view uri, const std::string_view original_file_name) -> bool {
    //     std::cout << "Found " << original_file_name << " at " << uri << std::endl;
    //     return true;
    // }, std::chrono::seconds(10), GNUNET_FS_SEARCH_OPTION_NONE, 1);

    // gnunetpp::FS::download(cfg, "gnunet://fs/chk/N59XZ8KPMQ0975JBTPV9AGEAD5T7V694QYWNK21683Q74TTRMQB2CHW4AZVTM3A5NFC57K0N6PD5EGCGMJABTZ6HKMV9ZC1T52FTVSG.2RJ19QYFPBJ2TBMZSNECXP9KHDTX90B6ZCTBSJYQKPK016156HNCPE5RJMNEM3A1NTRHMVWK8GCJ1MVG4S25F8A4TW1S70PCDMSG94R.2778934"
    //     , "test.jpg"
    //     , [](gnunetpp::FS::DownloadStatus status) {
    //         std::cout << "Download status: " << static_cast<int>(status) << std::endl;
    //         if(status == gnunetpp::FS::DownloadStatus::Completed)
    //             gnunetpp::shutdown();
    // }, 1);

    // gnunetpp::FS::scan(cfg, "CMakeCache.txt", [](GNUNET_FS_DirScanner* ds, const std::string& filename, bool is_dir, GNUNET_FS_DirScannerProgressUpdateReason reason) {
    //     if(reason == GNUNET_FS_DIRSCANNER_FINISHED) {
    //         std::cout << "Scan finished" << std::endl;
    //         gnunetpp::shutdown();
    //     }
    //     if(reason == GNUNET_FS_DIRSCANNER_FILE_START) {
    //         std::cout << "scanning " << filename << std::endl;
    //     }
    // });

    std::string publish_file = "test3.txt";
    gnunetpp::FS::publish(cfg, publish_file, {"test_file"}, [publish_file](const std::string& uri) {
        std::cout << "Published  " << publish_file <<" to " << uri << std::endl;
    });
}

int main()
{
    gnunetpp::run(service);
}
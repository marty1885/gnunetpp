#include <gnunetpp.hpp>
#include <iostream>

std::shared_ptr<gnunetpp::FS> fs;
void service(const GNUNET_CONFIGURATION_Handle* cfg)
{
    fs = std::make_shared<gnunetpp::FS>(cfg);
    fs->search({"coscup"}, [](const std::string_view payload) -> bool {
	std::cout << "Found `" << payload << "`!" << std::endl;
	return true;
    }, std::chrono::seconds(10));
}

int main()
{
    gnunetpp::run(service);
}
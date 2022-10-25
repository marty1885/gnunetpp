#include <gnunetpp.hpp>
#include <iostream>

std::shared_ptr<gnunetpp::identity::IdentityService> identity;

void service(const GNUNET_CONFIGURATION_Handle* cfg)
{
    identity = std::make_shared<gnunetpp::identity::IdentityService>(cfg);
    identity->create_identity("test", [](const GNUNET_IDENTITY_PrivateKey& key, const std::string& err) {
	if(err.empty()) {
	    std::cout << "Created identity: " << gnunetpp::identity::to_string(key) << std::endl;
	} else {
	    std::cout << "Error: " << err << std::endl;
	}
	gnunetpp::shutdown();
    });
}

int main()
{
    gnunetpp::run(service);
}
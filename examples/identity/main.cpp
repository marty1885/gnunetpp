#include "CLI/App.hpp"
#include "CLI/Formatter.hpp"
#include "CLI/Config.hpp"

#include <gnunetpp.hpp>
#include <iostream>

bool run_create;
bool run_delete;
bool run_get;

std::string identity_name;
std::string key_type;

std::shared_ptr<gnunetpp::identity::IdentityService> identity;

void service(const GNUNET_CONFIGURATION_Handle* cfg)
{
    using namespace gnunetpp::identity;
    // IdentityService is our interface to the GNUnet identity system
    identity = std::make_shared<IdentityService>(cfg);

    if(run_create) {
        // GNUnet supports 2 key types: ECDSA and EdDSA. EdDSA has limited support (ex. no FS) so
        // the default is ECDSA.
        GNUNET_IDENTITY_KeyType type = GNUNET_IDENTITY_TYPE_ECDSA;
        if(key_type == "EdDSA")
            type = GNUNET_IDENTITY_TYPE_EDDSA;
        else if(key_type != "ECDSA") {
            std::cerr << "Invalid key type: " << key_type << ". Must be ECDSA or EdDSA" << std::endl;
            abort();
        }

        // Create a new identity. The callback will be called when the identity is created or failed.
        identity->create_identity(identity_name, [](const GNUNET_IDENTITY_PrivateKey& key, const std::string& err) {
            if(err.empty())
                std::cout << "Created identity " << identity_name << ": " << to_string(get_public_key(key)) << std::endl;
            else
                std::cout << "Error: " << err << std::endl;
            gnunetpp::shutdown();
        }, type);
    }
    
    else if(run_get) {
        // Ego is GNUnet's name for identities. This function will lookup an identity by name and
        // calls the callback with the result.
        lookup_ego(cfg, identity_name, [](GNUNET_IDENTITY_Ego* ego) {
            if(ego != nullptr) {
                auto pk = get_public_key(ego);
                auto key_type = get_key_type(ego);
                std::cout << "Found ego: " << to_string(pk) << " - " << to_string(key_type) << std::endl;
            }
            else
                std::cout << "Ego not found" << std::endl;
            gnunetpp::shutdown();
        });
    }

    else if(run_delete) {
        // Delete an identity. The callback will be called when the identity is deleted (or failed).
        // Be careful with this function,
        identity->delete_identity(identity_name, [](const std::string& err) {
            if(err.empty())
                std::cout << "Deleted identity " << identity_name << std::endl;
            else
                std::cout << "Error: " << err << std::endl;
            gnunetpp::shutdown();
        });
    }
}

int main(int argc, char** argv)
{
    CLI::App app{"gnunetpp-identity"};
    app.require_subcommand(1);
    auto create_identity = app.add_subcommand("create", "Create a new identity");
    auto delete_identity = app.add_subcommand("delete", "Delete an identity");
    auto get = app.add_subcommand("get", "Get an identity (ego)");

    create_identity->add_option("name", identity_name, "Name of the identity")->required();
    create_identity->add_option("-t,--type", key_type, "Key type. Valid values are EDCSA/EdDSA. EdDSA is experimental")
        ->default_val("DCESA");

    delete_identity->add_option("name", identity_name, "Name of the identity")->required();
    get->add_option("name", identity_name, "Name of the identity")->required();

    CLI11_PARSE(app, argc, argv);
    run_create = create_identity->parsed();
    run_delete = delete_identity->parsed();
    run_get = get->parsed();

    gnunetpp::run(service);
}
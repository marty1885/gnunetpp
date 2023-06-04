#include "gnunetpp-peerinfo.hpp"
#include "gnunetpp-crypto.hpp"
#include <iostream>

using namespace gnunetpp;

PeerInfo::PeerInfo(const GNUNET_CONFIGURATION_Handle* cfg)
{
    handle = GNUNET_PEERINFO_connect(cfg);
    registerService(this);
}

void PeerInfo::shutdown()
{
    if(handle) {
        GNUNET_PEERINFO_disconnect(handle);
        handle = nullptr;
    }
}

PeerInfo::~PeerInfo()
{
    shutdown();
    removeService(this);
}

struct PeerInfoCallbackPack
{
    std::function<void(const std::set<GNUNET_PeerIdentity> addr)> callback;
    std::function<void(const std::string_view)> errorCallback;
    std::set<GNUNET_PeerIdentity> peers;
};

void PeerInfo::peers(std::function<void(const std::set<GNUNET_PeerIdentity> peer_ids)> callback, std::function<void(const std::string_view)> errorCallback)
{
    auto pack = new PeerInfoCallbackPack{std::move(callback), std::move(errorCallback)};
    auto ic = GNUNET_PEERINFO_iterate(handle, GNUNET_NO, nullptr
        , [](void *cls,
            const struct GNUNET_PeerIdentity *peer,
            const struct GNUNET_HELLO_Message *hello,
            const char *err_msg) {
        auto pack = reinterpret_cast<PeerInfoCallbackPack*>(cls);
        if(err_msg) {
            pack->errorCallback("Error in peer info");
            delete pack;
        }
        else if(peer == nullptr) {
            pack->callback(std::move(pack->peers));
            delete pack;
        }
        else {
            pack->peers.insert(*peer);
        }
    }, pack);
    if(!ic) {
        errorCallback("Failed to iterate peer info");
    }
}

Task<std::set<GNUNET_PeerIdentity>> PeerInfo::peers()
{
    struct PeerInfoAwaiter : public EagerAwaiter<std::set<GNUNET_PeerIdentity>>
    {
        PeerInfoAwaiter(PeerInfo& pi)
            : pi(pi)
        {
            pi.peers([this](const std::set<GNUNET_PeerIdentity> peers) {
                setValue(peers);
            }, [this](const std::string_view err) {
                setException(std::make_exception_ptr(std::runtime_error(err.data())));
            });
        }

        PeerInfo& pi;
    };
    co_return co_await PeerInfoAwaiter(*this);
}

void PeerInfo::addPeer(const std::shared_ptr<GNUNET_HELLO_Message>& hello, std::function<void()> callback)
{
    // TODO: The callback may not be called if PeerInfo is destroyed before the callback is called.
    //     Due to this operation is automatically cancelled.
    auto ic = GNUNET_PEERINFO_add_peer(handle, hello.get(), [](void *cls) {
        auto callback = reinterpret_cast<std::function<void()>*>(cls);
        (*callback)();
        delete callback;
    }, new std::function<void()>(std::move(callback)));
}

Task<> PeerInfo::addPeer(std::shared_ptr<GNUNET_HELLO_Message> hello)
{
    struct PeerInfoAwaiter : public EagerAwaiter<>
    {
        PeerInfoAwaiter(PeerInfo& pi, std::shared_ptr<GNUNET_HELLO_Message> hello)
            : hello(std::move(hello))
        {
            pi.addPeer(this->hello, [this]() {
                setValue();
            });
        }

        std::shared_ptr<GNUNET_HELLO_Message> hello;
    };
    co_return co_await PeerInfoAwaiter(*this, std::move(hello));
}

struct HelloCallbackPack
{
    std::function<void(std::shared_ptr<GNUNET_HELLO_Message> hello)> callback;
    GNUNET_TRANSPORT_HelloGetHandle* handle;
    const GNUNET_CONFIGURATION_Handle* cfg;
};

void hello_callback(void *cls, const GNUNET_MessageHeader *msg)
{
    auto pack = reinterpret_cast<HelloCallbackPack*>(cls);
    auto hello = reinterpret_cast<const GNUNET_HELLO_Message*>(msg);
    if(!hello) {
        GNUNET_TRANSPORT_hello_get_cancel(pack->handle);
        pack->callback(nullptr);
        delete pack;
        return;
    }
    auto my_id = crypto::myPeerIdentity(pack->cfg);

    if(GNUNET_HELLO_get_id(hello, &my_id) != GNUNET_OK) {
        GNUNET_TRANSPORT_hello_get_cancel(pack->handle);
        pack->callback(nullptr);
        delete pack;
        return;
    }
    auto copy = (GNUNET_HELLO_Message*)GNUNET_copy_message(msg);
    auto res = std::shared_ptr<GNUNET_HELLO_Message>(copy, [](GNUNET_HELLO_Message* msg) {
        GNUNET_free(msg);
    });
    pack->callback(res);
    
    GNUNET_TRANSPORT_hello_get_cancel(pack->handle);
    delete pack;
}

void gnunetpp::helloMessage(const GNUNET_CONFIGURATION_Handle *cfg, GNUNET_TRANSPORT_AddressClass ac, std::function<void(std::shared_ptr<GNUNET_HELLO_Message> hello)> callback)
{
    auto pack = new HelloCallbackPack;
    pack->callback = std::move(callback);
    pack->cfg = cfg;
    pack->handle = GNUNET_TRANSPORT_hello_get(cfg, ac, hello_callback, pack);
}

Task<std::shared_ptr<GNUNET_HELLO_Message>> gnunetpp::helloMessage(const GNUNET_CONFIGURATION_Handle *cfg, GNUNET_TRANSPORT_AddressClass ac)
{
    struct HelloAwaiter : public EagerAwaiter<std::shared_ptr<GNUNET_HELLO_Message>>
    {
        HelloAwaiter(const GNUNET_CONFIGURATION_Handle* cfg, GNUNET_TRANSPORT_AddressClass ac)
        {
            gnunetpp::helloMessage(cfg, ac, [this](std::shared_ptr<GNUNET_HELLO_Message> hello) {
                if(hello) {
                    setValue(hello);
                }
                else {
                    setException(std::make_exception_ptr(std::runtime_error("Failed to get hello message")));
                }
            });
        }
    };
    co_return co_await HelloAwaiter(cfg, ac);
}

using time_point = std::chrono::system_clock::time_point;
static std::string serializeTimePoint( const time_point& time, const std::string& format)
{
    std::time_t tt = std::chrono::system_clock::to_time_t(time);
    std::tm tm = *std::gmtime(&tt); //GMT (UTC)
    std::stringstream ss;
    ss << std::put_time( &tm, format.c_str() );
    return ss.str();
}

#include <iostream>

std::string gnunetpp::to_string(const std::shared_ptr<GNUNET_HELLO_Message> hello)
{
    std::string uri = GNUNET_HELLO_URI_PREFIX + crypto::to_string(hello->publicKey);
    GNUNET_PeerIdentity peer;
    peer.public_key = hello->publicKey;

    int i = 0;
    uint8_t* data = (uint8_t*)hello.get() + sizeof(GNUNET_HELLO_Message);
    auto msg_size = ntohs(hello->header.size) - sizeof(GNUNET_HELLO_Message);
    while(i < msg_size) {
        // now we start reading bytes. God damn GNUnet. The format is documented in 
        // gnunet_hello_lib.h. It looks like this
        // |-----------------------|-------------------------|---------------------------|--------------------------|
        //   transport (NULL term)   address len (2 byte BE)   experation (BE, 8 bytes)     address (address len)
        std::string transport_name;
        GNUNET_HELLO_Address* address = nullptr;
        GNUNET_TIME_AbsoluteNBO expiration;
        uint16_t address_len;

        // read transport name
        char* transport_name_end = (char*)memchr(data + i, '\0', msg_size - i);
        if(!transport_name_end) {
            throw std::runtime_error("Invalid hello message. Transport name not null terminated until end of message");
        }
        size_t transport_name_len = transport_name_end - (char*)data - i;
        transport_name = std::string((char*)data + i, transport_name_len);
        i += transport_name_len + 1; // +1 for null terminator

        // read address len
        if (i + 2 > msg_size) {
            throw std::runtime_error("Invalid hello message. Address len out of bounds");
        }
        memcpy(&address_len, data + i, 2); // handle possible alignment issues
        address_len = ntohs(address_len);
        i += 2;

        // read expiration
        if (i + 8 > msg_size) {
            throw std::runtime_error("Invalid hello message. Expiration out of bounds");
        }
        memcpy(&expiration, data + i, 8); // handle possible alignment issues
        expiration.abs_value_us__ = GNUNET_ntohll(expiration.abs_value_us__);
        i += sizeof(GNUNET_TIME_AbsoluteNBO);
        auto expiration_time = std::chrono::system_clock::time_point(std::chrono::seconds(expiration.abs_value_us__ / 1000000));
        auto expiration_time_str = serializeTimePoint(expiration_time, "%Y%m%d%H%M%S");

        // read address
        if (i + address_len > msg_size) {
            throw std::runtime_error("Invalid hello message. Address out of bounds");
        }
        address = GNUNET_HELLO_address_allocate(&peer, transport_name.c_str(), data + i, address_len, GNUNET_HELLO_ADDRESS_INFO_NONE);
        i += address_len;
        std::optional<std::string> address_str;
        // TCP IPv4. Struct IPv4TcpAddress is not exported by GNUnet but we can read the source code. Duh.
        // struct IPv4TcpAddress {
        //    uint32_t options;
        //    uint32_t ipv4_addr; // Network byte order
        //    uint16_t port;      // Network byte order
        // };
        if(transport_name == "tcp" && address->address_length == 10) {
            uint32_t ipv4_addr;
            uint16_t port;
            uint32_t options;
            uint8_t* begin = (uint8_t*)address->address;
            memcpy(&options, begin, 4);
            memcpy(&ipv4_addr, begin + 4, 4);
            memcpy(&port, begin + 8, 2);
            options = ntohl(options);
            ipv4_addr = ntohl(ipv4_addr);
            port = ntohs(port);
            std::string ipv4_addr_str = std::to_string((ipv4_addr >> 24) & 0xFF) + "." + std::to_string((ipv4_addr >> 16) & 0xFF) + "." + std::to_string((ipv4_addr >> 8) & 0xFF) + "." + std::to_string(ipv4_addr & 0xFF);
            address_str = "tcp." + std::to_string(options) + "." + ipv4_addr_str + ":" + std::to_string(port);
        }
        // TCP IPv6. Same deal. Because GNUnet does not expose plugins to the public API we have to read the source code.
        // struct IPv6TcpAddress {
        //    uint32_t options;
        //    in6_addr ipv6_addr;
        //    uint16_t port;        // Network byte order
        // };
        else if(transport_name == "tcp" && address->address_length == 22) {
            uint32_t options;
            in6_addr ipv6_addr;
            uint16_t port;
            uint8_t* begin = (uint8_t*)address->address;
            memcpy(&options, begin, 4);
            memcpy(&ipv6_addr, begin + 4, 16);
            memcpy(&port, begin + 20, 2);
            options = ntohl(options);
            port = ntohs(port);
            char ipv6_addr_str[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, &ipv6_addr, ipv6_addr_str, INET6_ADDRSTRLEN);
            address_str = "tcp." + std::to_string(options) + ".[" + ipv6_addr_str + "]:" + std::to_string(port);
        }
        // TODO: Add support to bluetooth, udp, unix, etc.


        GNUNET_HELLO_address_free(address);
        if(!address_str) {
            std::cout << "GNUnet++ cannot parse address for transport " << transport_name << std::endl;
            continue;
        }

        uri += "+" + expiration_time_str + "+" + transport_name + ":" + *address_str;
        // break;
    }
    return uri;
}

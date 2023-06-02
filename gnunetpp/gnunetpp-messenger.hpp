#pragma once

#include "inner/coroutine.hpp"
#include "gnunetpp-crypto.hpp"
#include "inner/Infra.hpp"

#include <gnunet/gnunet_messenger_service.h>

#include <string>
#include <map>
#include <variant>
#include <optional>

namespace gnunetpp
{

struct JoinMessage
{
    GNUNET_IDENTITY_PublicKey identity;
};

struct LeaveMessage
{
};

struct InviteMessage
{
    GNUNET_PeerIdentity door;
    GNUNET_HashCode key;
};

struct TextMessage
{
    std::string text;
};

struct FileMessage
{
    std::string filename;
    GNUNET_HashCode hash;
    std::vector<uint8_t> data;
};

struct NameChangeMessage
{
    std::string name;
};

struct NewDoorMessage
{
    GNUNET_PeerIdentity door;
};

struct Contact
{
    std::string name() const;
    std::optional<GNUNET_IDENTITY_PublicKey> key() const;

    const GNUNET_MESSENGER_Contact* contact = nullptr;
};

struct Message
{
    using MessageType = std::variant<JoinMessage, LeaveMessage, InviteMessage, TextMessage, FileMessage, NameChangeMessage, NewDoorMessage>;

    // sender of this message
    GNUNET_ShortHashCode sender_id;
    // When was this message sent
    std::chrono::system_clock::time_point timestamp;
    // The ID of this message
    GNUNET_HashCode id;
    // Who sent this message
    Contact sender;
    // Which kind of message is this
    GNUNET_MESSENGER_MessageKind kind;
    // Actual message
    MessageType message;
};

struct Room : public NonCopyable
{
    Room(GNUNET_MESSENGER_Room* room) : room(room) {}
    virtual ~Room() { if(room != nullptr) GNUNET_MESSENGER_close_room(room); }
    GNUNET_HashCode getId() const;
    Room(Room&& other) : room(other.room) { other.room = nullptr; }
    Room& operator=(Room&& other) { room = other.room; other.room = nullptr; return *this; }

    void sendMessage(const std::string& value);
    void sendPrivateMessage(const std::string& value, Contact contact);
    void setReadMessageCallback(std::function<void(const Message&)> cb) { recv_cb = std::move(cb); }

    GNUNET_MESSENGER_Room* room = nullptr;
    std::function<void(const Message&)> recv_cb;
};

struct Messenger : public Service
{
    Messenger(const GNUNET_CONFIGURATION_Handle* cfg, const std::string& ego_name = "");
    virtual void shutdown() override;
    ~Messenger();

    std::shared_ptr<Room> openRoom(const GNUNET_HashCode& id);
    std::shared_ptr<Room> enterRoom(const GNUNET_HashCode& id, const GNUNET_PeerIdentity& door, bool open_as_door = true);

    bool setName(const std::string& name);

    std::string getEgoName() const;

    cppcoro::task<> waitForInit();

    GNUNET_MESSENGER_Handle* handle = nullptr;
    bool identity_set = false;
    std::vector<std::function<void()>> init_cbs; 

    std::map<GNUNET_MESSENGER_Room*, std::weak_ptr<Room>> rooms;
};

}
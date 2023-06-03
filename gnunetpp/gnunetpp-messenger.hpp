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

    /**
     * @brief Send a message to all members of this room
     * 
     * @param value The message to send
    */
    void sendMessage(const std::string& value);
    /**
     * @brief Send a private message to a specific member of this room
     * 
     * @param value The message to send
    */
    void sendPrivateMessage(const std::string& value, Contact contact);
    /**
     * @brief Set a callback for when a message is received
     * 
     * @param cb The callback to set
    */
    void setReadMessageCallback(std::function<void(const Message&)> cb) { recv_cb = std::move(cb); }

    GNUNET_MESSENGER_Room* room = nullptr;
    std::function<void(const Message&)> recv_cb;
};

struct Messenger : public Service
{
    /**
     * @brief Create a new messenger
     * @param cfg The GNUnet configuration handle
     * @param ego_name The name of your ego. If empty, an anonymous ego will be used
    */
    Messenger(const GNUNET_CONFIGURATION_Handle* cfg, const std::string& ego_name = "");
    virtual void shutdown() override;
    ~Messenger();

    /**
     * @brief "Opens" a room so your node becomes a door for this room. If your node is not already a member of this room,
     *       a new room will be created.
     * 
     * @param id The ID of the room to open
    */
    std::shared_ptr<Room> openRoom(const GNUNET_HashCode& id);
    /**
     * @brief "Enters" a room via a door.
     * 
     * @param id The ID of the room to enter
     * @param door The door to enter through
     * @param open_as_door If true, your node will become a door for this room
     * 
     * @note entering a room with open_as_door = true is equlicent to setting it to false then calling openRoom with the same ID 
    */
    std::shared_ptr<Room> enterRoom(const GNUNET_HashCode& id, const GNUNET_PeerIdentity& door, bool open_as_door = true);

    /**
     * Set the name of your ego viewd by other nodes
    */
    bool setName(const std::string& name);

    /**
     * @return the name of your ego used during initialization
    */
    std::string getEgoName() const;

    /**
     * @brief Wait for the messenger to be initialized
    */
    cppcoro::task<> waitForInit();

    GNUNET_MESSENGER_Handle* handle = nullptr;
    bool identity_set = false;
    std::vector<std::function<void()>> init_cbs; 

    std::map<GNUNET_MESSENGER_Room*, std::weak_ptr<Room>> rooms;
};

}
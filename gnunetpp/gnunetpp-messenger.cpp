#include "gnunetpp-messenger.hpp"
#include "gnunetpp-scheduler.hpp"

using namespace gnunetpp;

#include <iostream>
#include <any>

void message_callback (void *cls,
                        GNUNET_MESSENGER_Room *room,
                        const GNUNET_MESSENGER_Contact *sender,
                        const GNUNET_MESSENGER_Message *message,
                        const GNUNET_HashCode *hash,
                        enum GNUNET_MESSENGER_MessageFlags flags)
{
    auto self = (Messenger*)cls;
    GNUNET_assert(self != nullptr);

    auto it = self->rooms.find(room);
    if(it == self->rooms.end()) {
        std::cerr << "Received message for unknown room. This is a bug." << std::endl;
        return;
    }
    auto ptr = it->second.lock();
    if(ptr == nullptr) {
        std::cerr << "Received message for destroyed room. This is a bug." << std::endl;
        return;
    }

    Message::MessageType msg;
    switch (message->header.kind)
    {
    case GNUNET_MESSENGER_KIND_TEXT:
        msg = TextMessage{std::string(message->body.text.text)};
        break;
    case GNUNET_MESSENGER_KIND_JOIN:
        msg = JoinMessage{message->body.join.key};
        break;
    case GNUNET_MESSENGER_KIND_LEAVE:
        msg = LeaveMessage{};
        break;
    case GNUNET_MESSENGER_KIND_INVITE:
        msg = InviteMessage{message->body.invite.door, message->body.invite.key};
        break;
    case GNUNET_MESSENGER_KIND_NAME:
        msg = NameChangeMessage{std::string(message->body.name.name)};
        break;
    case GNUNET_MESSENGER_KIND_PEER:
        msg = NewDoorMessage{message->body.peer.peer};
        break;
    
    default:
        break;
    }
    if(msg.index() == std::variant_npos)
        return;
    
    auto time_us = GNUNET_ntohll(message->header.timestamp.abs_value_us__);
    Message res {
        .sender_id = message->header.sender_id,
        .timestamp = std::chrono::system_clock::from_time_t(time_us/1000/1000),
        .id = *hash,
        .sender = Contact{sender},
        .kind = message->header.kind,
        .message = std::move(msg)
    };
    ptr->recv_cb(res);
}

void identity_callback (void *cls, GNUNET_MESSENGER_Handle *handle)
{
    auto self = (Messenger*)cls;
    self->identity_set = true;

    for(auto& cb : self->init_cbs)
        cb();
}

Messenger::Messenger(const GNUNET_CONFIGURATION_Handle* cfg, const std::string& ego_name)
{
    const char* name = nullptr;
    if(ego_name.empty()) {
        name = ego_name.c_str();
    }
    handle = GNUNET_MESSENGER_connect(cfg, name, identity_callback, this, message_callback, this);
    GNUNET_assert(handle != nullptr);
    registerService(this);
}

Messenger::~Messenger()
{
    shutdown();
    removeService(this);
}

std::shared_ptr<Room> Messenger::openRoom(const GNUNET_HashCode &id)
{
    auto room = GNUNET_MESSENGER_open_room(handle, &id);
    auto it = rooms.find(room);
    if(it != rooms.end()) {
        auto ptr = it->second.lock();
        GNUNET_assert(ptr != nullptr);
        return ptr;
    }

    auto ptr = std::shared_ptr<Room>(new Room(room), [room, this] (Room* ptr) {
        delete ptr;
        rooms.erase(room);
    });
    rooms[room] = ptr;
    return ptr;
}

std::shared_ptr<Room> Messenger::enterRoom(const GNUNET_HashCode &id, const GNUNET_PeerIdentity &door, bool open_as_door)
{
    auto room = GNUNET_MESSENGER_enter_room(handle, &door, &id);
    if(room == nullptr)
        return nullptr;
    if(open_as_door)
        GNUNET_MESSENGER_open_room(handle, &id);
    auto it = rooms.find(room);
    if(it != rooms.end()) {
        auto ptr = it->second.lock();
        GNUNET_assert(ptr != nullptr);
        return ptr;
    }

    auto ptr = std::shared_ptr<Room>(new Room(room), [room, this] (Room* ptr) {
        delete ptr;
        rooms.erase(room);
    });
    rooms[room] = ptr;
    return ptr;
}

bool Messenger::setName(const std::string &name)
{
    return GNUNET_MESSENGER_set_name(handle, name.c_str()) ==  GNUNET_YES;
}

void Messenger::shutdown()
{
    if(handle != nullptr)
    {
        GNUNET_MESSENGER_disconnect(handle);
        handle = nullptr;
    }
}

std::string Messenger::getEgoName() const
{
    auto name = GNUNET_MESSENGER_get_name(handle);
    // NULL means using anonymous identity
    if(name == nullptr)
        return "";
    return name;
}

Task<> Messenger::waitForInit()
{
    struct Awaiter : public EagerAwaiter<>
    {
        Awaiter(Messenger* messenger) : messenger(messenger) {
            if(messenger->identity_set)
                setValue();
            messenger->init_cbs.push_back([this] () {
                setValue();
            });
        }

        Messenger* messenger;
    };

    co_return co_await Awaiter(this);
}

GNUNET_HashCode Room::getId() const
{
    return *GNUNET_MESSENGER_room_get_key(room);
}

// using UserSendibleMessage = std::variant<TextMessage, FileMessage, NameChangeMessage, InviteMessage>;
[[nodiscard]]
static std::pair<GNUNET_MESSENGER_Message, std::any> message_convert(const UserSendibleMessage &value)
{
    GNUNET_MESSENGER_Message msg;
    if(value.index() == std::variant_npos)
        return {msg, std::any()};

    std::shared_ptr<std::string> holder = nullptr;

    switch(value.index()) {
        case 0:
            msg.header.kind = GNUNET_MESSENGER_KIND_TEXT;
            // HACK: We need to keep the string alive until the message is sent
            holder = std::make_shared<std::string>(std::get<0>(value).text);
            msg.body.text.text = (char*)holder->c_str();
            break;
        case 1:
            // Don't know how to send files yet
            throw std::runtime_error("Sending files is not supported yet");
            break;
        case 2:
            msg.header.kind = GNUNET_MESSENGER_KIND_NAME;
            holder = std::make_shared<std::string>(std::get<2>(value).name);
            msg.body.name.name = (char*)holder->c_str();
            break;
        case 3:
            msg.header.kind = GNUNET_MESSENGER_KIND_INVITE;
            msg.body.invite.key = std::get<3>(value).key;
            msg.body.invite.door = std::get<3>(value).door;
            break;
        default:
            // Should never happen
            throw std::runtime_error("Unknown message type");
    }

    std::any res = std::move(holder);
    return {msg, res};
}

void Room::sendMessage(const std::string &value)
{
    auto [msg, _] = message_convert(TextMessage{value});
    GNUNET_MESSENGER_send_message(room, &msg, nullptr);
}

void Room::sendMessage(const UserSendibleMessage &value)
{
    auto [msg, _] = message_convert(value);
    GNUNET_MESSENGER_send_message(room, &msg, nullptr);
}

void Room::sendPrivateMessage(const std::string &value, Contact contact)
{
    auto [msg, _] = message_convert(TextMessage{value});
    GNUNET_MESSENGER_send_message(room, &msg, contact.contact);
}

void Room::sendPrivateMessage(const UserSendibleMessage &value, Contact contact)
{
    auto [msg, _] = message_convert(value);
    GNUNET_MESSENGER_send_message(room, &msg, contact.contact);
}

int Room::forEachMember(std::function<void(const Contact &)> cb)
{
    auto ptr = new std::function<void(const Contact &)>(cb);
    // IMPORTANT: this method is synchronous, so we can safely delete the pointer after the call
    int num_members = GNUNET_MESSENGER_iterate_members(room, 
    [] (void* cls, GNUNET_MESSENGER_Room* room, const GNUNET_MESSENGER_Contact* contact) -> int {
        auto cb = (std::function<void(const Contact &)>*)cls;
        (*cb)(Contact{contact});
        return GNUNET_OK;
    }, ptr);
    delete ptr;
    return num_members;
}

std::vector<Contact> Room::members()
{
    std::vector<Contact> contacts;
    int n = forEachMember([&contacts] (const Contact& contact) {
        contacts.push_back(contact);
    });
    GNUNET_assert(n == contacts.size());
    return contacts;
}

std::string Contact::name() const
{
    auto name = GNUNET_MESSENGER_contact_get_name(contact);
    if(name == nullptr)
        return "";
    return name;
}

std::optional<GNUNET_IDENTITY_PublicKey> Contact::key() const
{
    auto pk = GNUNET_MESSENGER_contact_get_key(contact);
    if(pk == nullptr)
        return std::nullopt;
    return *pk;
}

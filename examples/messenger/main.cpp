#include "CLI/App.hpp"
#include "CLI/Formatter.hpp"
#include "CLI/Config.hpp"

#include "gnunetpp-messenger.hpp"
#include "gnunetpp-identity.hpp"
#include "gnunetpp-scheduler.hpp"

#include <iostream>

using namespace gnunetpp;
using namespace std::chrono_literals;

std::string door_str;
std::string room_str;
std::string room_hash_str;
std::string identity_name;
bool open_as_door = false;

using time_point = std::chrono::system_clock::time_point;
std::string serializeTimePoint( const time_point& time, const std::string& format)
{
    std::time_t tt = std::chrono::system_clock::to_time_t(time);
    std::tm tm = *std::gmtime(&tt); //GMT (UTC)
    std::stringstream ss;
    ss << std::put_time( &tm, format.c_str() );
    return ss.str();
}

Task<> service(const GNUNET_CONFIGURATION_Handle* cfg)
{
    if(!identity_name.empty()) {
        auto identity = co_await getEgo(cfg, identity_name);
        if(!identity)
            throw std::runtime_error("Identity " + identity_name + " not found");
    }
    auto messenger = std::make_shared<Messenger>(cfg, identity_name);
    // Messenger needs time to retrieve identities and initialize. Wait for it.
    co_await messenger->waitForInit();

    std::string disp_name = identity_name;
    if(disp_name.empty())
        disp_name = "<anonymous>";
    std::cout << "Welcome to gnunetpp-messenger '" << disp_name <<"'!" << std::endl;
    std::cout << "Type you messange then press enter to send it. The special message /exit exits the program" << std::endl;
    std::cout << std::endl;

    // Now we can open or enter the room. Note that "open" menas we create a room on our host if not already existing. Then
    // we become a door for this room. On the other hand, if a room already exists somewhere on the netwotrk, we can enter
    // it via a door (which is a peer that opened the room) and optionally become a door for this room as well.
    std::shared_ptr<Room> room;
    if(door_str.empty()) {
        room = messenger->openRoom(crypto::hash(room_str));
        if(!room)
            throw std::runtime_error("Failed to open room " + room_str);
        std::cout << "* Room opened. You are connecting from peer " << crypto::to_string(crypto::myPeerIdentity(cfg)) << std::endl;
    }
    else {
        std::cout << "* Connecting to room " << room_str << " via peer " << door_str << std::endl;
        room = messenger->enterRoom(crypto::hash(room_str), crypto::peerIdentity(door_str), open_as_door);
        if(!room)
            throw std::runtime_error("Room " + room_str + " not found on peer " + door_str);
    }

    // On each message received, we print it to the console.
    room->setReadMessageCallback([] (const Message& message) {
        std::string disp_name = message.sender.name();
        if(disp_name.empty())
            disp_name = "<unknown>";
        auto time_str = serializeTimePoint(message.timestamp, "UTC %Y-%m-%d %H:%M:%S");
        // The format of the header is: [timestamp sender_id] sender_name:
        std::string header = "[\033[90m" + time_str + "\033[0m " + crypto::to_string_short(message.sender_id) + "] " + disp_name + ": ";

        // There's a wide variety of message types. Most of them infroamtional or control messages. We print some of the more important ones.
        // In our case, all contol messages are prefixed with an asterisk. And the action taken is printed also in asterisks.
        // For example: * [UTC 2023-06-02 10:47:56 VJ5P2S] <unknown>: *joined the room*
        // Whereas normal messages are printed without asterisks.
        // [UTC 2023-06-02 10:47:59 SYS3JS] clehaxze: Hello World!
        if(message.kind == GNUNET_MESSENGER_KIND_TEXT)
            std::cout << header << std::get<TextMessage>(message.message).text << std::endl;
        else if(message.kind == GNUNET_MESSENGER_KIND_JOIN)
            std::cout << "* " << header << "*joined the room*" << std::endl;
        else if(message.kind == GNUNET_MESSENGER_KIND_LEAVE)
            std::cout << "* " << header << "*left the room*" << std::endl;
        else if(message.kind == GNUNET_MESSENGER_KIND_INVITE) {
            auto& msg = std::get<InviteMessage>(message.message);
            std::cout << "* " << header << "*Invites you to join the room " << crypto::to_string_short(msg.key) << " via peer " << crypto::to_string_short(msg.door) << "*" << std::endl;
            std::cout << "* " << "Run `gnunetpp-messenger -d " << crypto::to_string(msg.door) << " -R " << crypto::to_string(msg.key) << "` to join the room" << std::endl;
        }
        else if(message.kind == GNUNET_MESSENGER_KIND_NAME) {
            std::cout << "* " << header << "*" << crypto::to_string_short(message.sender_id) << "\033[90m changed name to \033[0m"
                << std::get<NameChangeMessage>(message.message).name << "*" << std::endl;
        }
        else if(message.kind == GNUNET_MESSENGER_KIND_PEER) {
            std::cout << "* " << header << "*Opens a new door to this room on " << crypto::to_string(std::get<NewDoorMessage>(message.message).door) << "*" << std::endl;
        }
    });

    // Read from stdin and send each line as a message to the room.
    while(true) {
        std::string line = co_await scheduler::readStdin();
        if(!line.empty() && line.back() == '\n')
            line.pop_back();
        if(line == "/exit") {
            std::cout << "Exiting..." << std::endl;
            shutdown();
            co_return;
        }
        else if(line == "/list") {
            std::cout << "Members of room " << room_str << ":" << std::endl;
            auto members = room->members();
            for(auto& contact : members) {
                std::string disp_name = contact.name();
                std::string key = "<unknown>";
                if(contact.key())
                    key = to_string(*contact.key());
                if(disp_name.empty())
                    disp_name = "<unknown>";
                std::cout << "* " << disp_name << " (" << key << ")" << std::endl;
            }
        }
        // don't send empty messages
        else if(!line.empty())
            room->sendMessage(line);
    }
}

int main(int argc, char** argv)
{
    CLI::App app("Messaging on GNUnet", "gnunetpp-messenger");

    app.add_option("-d,--door", door_str, "Door to enter the room through");
    app.add_option("-r,--room", room_str, "Room to enter");
    app.add_option("-R,--room-hash", room_hash_str, "Room to enter as hash/ID");
    app.add_option("-i,--identity", identity_name, "Identity to use, defaults to be anonymous");
    app.add_flag("-o,--open", open_as_door, "Open your node as a door for others to join")->default_val(false);

    CLI11_PARSE(app, argc, argv);
    if(room_hash_str.empty() && room_str.empty())
        throw std::runtime_error("Room or Room ID must be specified");

    gnunetpp::start(service);
    return 0;
}
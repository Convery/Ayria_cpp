/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-12-08
    License: MIT
*/

#include "Common.hpp"

namespace Matchmaking
{
    std::vector<std::shared_ptr<Server_t>> Externalhosts{};
    std::shared_ptr<Server_t> Localhost{};

    // A list of all currently active servers on the local net.
    std::vector<std::shared_ptr<Server_t>> Externalservers()
    {
        const auto Localtime = time(nullptr) - Ayria::Global.Startuptimestamp;
        std::vector<std::shared_ptr<Server_t>> Activeservers{};
        for (const auto &Server : Externalhosts)
        {
            if ((Localtime - Server->Core->Timestamp) < 30)
            {
                Activeservers.push_back(Server);
            }
        }
        return Activeservers;
    }
    std::shared_ptr<Server_t> Localserver()
    {
        if (!Localhost)
        {
            Server_t Local{};
            Local.Core = Communication::getLocalhost();
            Localhost = std::make_shared<Server_t>(std::move(Local));
        }

        return Localhost;
    }

    // Notify the public about our node being dirty.
    void Broadcast()
    {
        if (Localserver()->Session.empty()) Communication::Broadcast("Matchmaking::Terminate", "");
        else Communication::Broadcast("Matchmaking::Update", Base64::Encode(Localserver()->Session.dump()));
    }

    // Callbacks on events by name.
    void addEventhandler(const std::string &Event, Eventcallback_t Callback)
    {
        Communication::addEventhandler(Event, [=](std::shared_ptr<Communication::Node_t> Sender, std::string &&Payload)
        {
            LABEL_RETRY:
            for (const auto &Server : Externalhosts)
            {
                if (Server->Core == Sender)
                {
                    return Callback(Server, std::move(Payload));
                }
            }

            Externalhosts.emplace_back(std::make_shared<Server_t>())->Core = Sender;
            goto LABEL_RETRY;
        });
    }

    // Standard matchmaking callbacks.
    void onUpdate(std::shared_ptr<Server_t> Sender, std::string &&Payload)
    {
        try
        {
            // Parse the payload-data as JSON, for some reason std::string doesn't decay properly.
            Sender->Session = nlohmann::json::parse(Base64::Decode(Payload).c_str());
        } catch (std::exception &) {}
    }
    void onTerminate(std::shared_ptr<Server_t> Sender, std::string &&)
    {
        Sender->Core->Timestamp = 0;
    }

    struct Startup
    {
        Startup()
        {
            addEventhandler("Matchmaking::Update", onUpdate);
            addEventhandler("Matchmaking::Terminate", onTerminate);
        }
    }; static Startup Loader{};
}

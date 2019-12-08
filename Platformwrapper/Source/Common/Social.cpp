/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-12-08
    License: MIT
*/

#include "Common.hpp"

namespace Social
{
    std::vector<std::shared_ptr<Social_t>> Friendly{};
    std::shared_ptr<Social_t> Localhost{};

    // List all clients on the network as friends.
    std::vector<std::shared_ptr<Social_t>> Friendlyclients()
    {
        // TODO(tcn): Filter by some kind of friends-list.
        return Friendly;
    }

    // Callbacks on events by name.
    void addEventhandler(const std::string &Event, Eventcallback_t Callback)
    {
        Communication::addEventhandler(Event, [=](std::shared_ptr<Communication::Node_t> Sender, std::string &&Payload)
        {
            LABEL_RETRY:
            for (const auto &Friend : Friendly)
            {
                if (Friend->Core == Sender)
                {
                    return Callback(Friend, std::move(Payload));
                }
            }

            Friendly.emplace_back(std::make_shared<Social_t>())->Core = Sender;
            goto LABEL_RETRY;
        });
    }

    // Standard social callbacks.
    static void onStartup(std::shared_ptr<Social_t> Sender, std::string &&)
    {
        if (!Localhost)
        {
            Social_t Local{};
            Local.Core = Communication::getLocalhost();
            Local.Set("UserID", Ayria::Global.UserID);
            Local.Set("Username", Ayria::Global.Username);
            Localhost = std::make_shared<Social_t>(std::move(Local));
        }

        // Share our information with the world.
        Communication::Broadcast("Social::Update", Base64::Encode(Localhost->Richpresence.dump()));
    }
    static void onUpdate(std::shared_ptr<Social_t> Sender, std::string &&Payload)
    {
        if (!Localhost) onStartup(Sender, "");

        try
        {
            // Parse the payload-data as JSON, for some reason std::string doesn't decay properly.
            Sender->Richpresence = nlohmann::json::parse(Base64::Decode(Payload).c_str());
        } catch (std::exception &) {}
    }

    struct Startup
    {
        Startup()
        {
            addEventhandler("Social::Update", onUpdate);
            addEventhandler("Social::Startup", onStartup);
        }
    }; static Startup Loader{};
}

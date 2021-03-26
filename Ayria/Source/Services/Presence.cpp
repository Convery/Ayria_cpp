/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-03-26
    License: MIT
*/

#include <Stdinclude.hpp>
#include <Global.hpp>

namespace Services::Presence
{
    Hashmap<std::string, LZString_t> Clientpresence;

    // Handle local client-requests.
    static void __cdecl Updatehandler(unsigned int NodeID, const char *Message, unsigned int Length)
    {
        const JSON::Object_t Keyvalues = JSON::Parse(std::string_view(Message, Length));
        const auto ClientID = Clientinfo::getClientID(NodeID);

        for (const auto &[Key, Value] : Keyvalues)
        {
            // Ensure that the values are available to the plugins.
            Backend::Database() << "REPLACE (ClientID, Key, Value) INTO Presence VALUES (?, ?, ?);"
                                << ClientID << Key << Value.get<std::string>();
        }
    }

    // Send requests to local clients.
    static void Sendupdate(const std::unordered_set<std::string> &&Keys)
    {
        JSON::Object_t Keyvalues{};

        if (Keys.empty())
        {
            for (const auto &[Key, Value] : Clientpresence)
                Keyvalues[Key] = (std::string)Value;
        }
        else
        {
            for(const auto &Key : Keys)
                Keyvalues[Key] = (std::string)Clientpresence[Key];
        }

        Backend::Network::Transmitmessage("Presence::Update", Keyvalues);
    }

    // JSON API access for the plugins.
    static std::string __cdecl Updatepresence(JSON::Value_t &&Request)
    {
        const JSON::Object_t Keyvalues = Request;
        std::unordered_set<std::string> Keys;
        Keys.reserve(Keyvalues.size());

        for (const auto &[Key, Value] : Keyvalues)
        {
            // Ensure that the values are available to the plugins.
            Backend::Database() << "REPLACE (ClientID, Key, Value) INTO Presence VALUES (?, ?, ?);"
                                << Global.ClientID << Key << Value.get<std::string>();

            // Save the precense for easy access.
            Clientpresence[Key] = Value;
            Keys.insert(Key);
        }

        Sendupdate(std::move(Keys));
        return "{}";
    }

    // Set up the service.
    void Initialize()
    {
        // Register the callback for client-requests.
        Backend::Network::Registerhandler("Presence::Update", Updatehandler);

        // Register the JSON API for plugins.
        Backend::API::addEndpoint("Presence::Update", Updatepresence);

        // Notify other clients once in a while.
        Backend::Enqueuetask(5000, []() { Sendupdate({}); });
    }
}

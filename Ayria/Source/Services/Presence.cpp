/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-03-26
    License: MIT
*/

#include <Stdinclude.hpp>
#include <Global.hpp>

namespace Services::Presence
{
    static Hashmap<std::string, LZString_t> Clientpresence;

    // Handle incoming client-updates.
    static void __cdecl Updatehandler(unsigned int NodeID, const char *Message, unsigned int Length)
    {
        const JSON::Object_t Keyvalues = JSON::Parse(std::string_view(Message, Length));
        const auto ClientID = Clientinfo::getClientID(NodeID);

        for (const auto &[Key, Value] : Keyvalues)
        {
            // Update the database so that the plugins can access this info.
            try { Backend::Database() << "REPLACE INTO Userpresence (ClientID, Key, Value) VALUES (?, ?, ?);"
                                      << ClientID << Key << Value.get<std::string>();
            } catch (...) {}
        }
    }

    // Notify the other clients about our state.
    static void Updatestate()
    {
        const auto Updates = Backend::getDatabasechanges("Userpresence");

        for (const auto &Item : Updates)
        {
            try
            {
                Backend::Database() << "SELECT * FROM Userpresence WHERE rowid = ?;" << Item
                >> [](uint32_t ClientID, const std::string &Key, const std::string &Value)
                   { if (ClientID == Global.ClientID) Clientpresence[Key] = Value; };
            } catch (...) {}
        }

        JSON::Object_t Keyvalues{};
        for (const auto &[Key, Value] : Clientpresence)
            Keyvalues[Key] = (std::string)Value;

        Backend::Network::Transmitmessage("Presence::Update", Keyvalues);
    }

    // Set up the service.
    void Initialize()
    {
        // Register the callback for client-requests.
        Backend::Network::Registerhandler("Presence::Update", Updatehandler);

        // Notify other clients once in a while.
        Backend::Enqueuetask(5000, Updatestate);
    }
}

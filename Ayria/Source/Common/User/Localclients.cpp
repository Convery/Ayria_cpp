/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-05-27
    License: MIT
*/

#include <Global.hpp>

namespace Clients
{
    std::unordered_set<sockaddr_in, decltype(FNV::Hash), decltype(FNV::Equal)> Greetednodes;
    std::vector<Client_t> Localclients;

    // Notify other users about us.
    void Sendclientinfo()
    {
        const auto Localclient = Clients::getSelf();
        auto Object = nlohmann::json::object();
        Object["Clientname"] = Localclient.Clientname;
        Object["Publickey"] = Localclient.Publickey;
        Object["ClientID"] = Localclient.ClientID;

        Networking::Core::Sendmessage("Ayria::Clientinfo", Object.dump());
    }

    // Listen for new clients on the network.
    void onClientinfo(sockaddr_in Client, const char *Content)
    {
        // Greet the node if we haven't.
        if (!Greetednodes.contains(Client)) [[unlikely]]
        {
            Greetednodes.insert(Client);
            Sendclientinfo();
        }

        try
        {
            Client_t Newclient;

            const auto Object = nlohmann::json::parse(Content);
            Newclient.Clientname = Object.value("Clientname", std::string());
            Newclient.Publickey = Object.value("Publickey", std::string());
            Newclient.ClientID = Object.value("ClientID", uint32_t());
            if (Newclient.ClientID == 0 || Newclient.Clientname.empty()) return;

            std::erase_if(Localclients, [Inputhash = Hash::FNV1a_64(&Newclient, sizeof(Newclient))](const auto &Item)
            {
                return Hash::FNV1a_64(&Item, sizeof(Item)) == Inputhash;
            });
            Localclients.emplace_back(Newclient);

        } catch (...) {}
    }

    // Helper for other code.
    std::vector<Client_t> *getLocalclients()
    {
        return &Localclients;
    }
}

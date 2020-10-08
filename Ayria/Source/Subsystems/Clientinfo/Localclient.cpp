/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-09-28
    License: MIT
*/

#include <Stdinclude.hpp>
#include <Global.hpp>

namespace Clientinfo
{
    Ayriaclient Localclient;
    std::unordered_set<Ayriaclient, decltype(FNV::Hash), decltype(FNV::Equal)> Localnetwork;

    // Fetch backend info.
    const Ayriaclient *getLocalclient()
    {
        return &Localclient;
    }

    namespace Internal
    {
        void Sendclientinfo()
        {
            const char *String;
            API::getLocalclient(&String);
            Auxiliary::Sendmessage(Hash::FNV1_32("Clientdiscovery"), String);
        }
        void __cdecl Discoveryhandler(std::string_view JSONString)
        {
            try
            {
                Ayriaclient Newclient{};

                const auto Object = nlohmann::json::parse(JSONString);
                Newclient.ClientID = Object.value("ClientID", uint32_t());
                Newclient.Locale = Object.value("Locale", std::wstring());
                Newclient.Username = Object.value("Username", std::wstring());

                Localnetwork.insert(std::move(Newclient));
            }
            catch (...) {}
        }

        static float Updatedelay = 0.0f;
        void UpdateLocalclient(float Deltatime)
        {
            Updatedelay -= Deltatime;
            if (Updatedelay < 0.0f)
            {
                // Announce our presence.
                Sendclientinfo();

                // TODO(tcn): Poll server.
                Updatedelay = 5.0f;
            }
        }
        void InitLocalclient()
        {
            Localclient.ClientID = 0xDEADC0DE;
            Localclient.Username = L"Ayria";
            Localclient.Locale = L"English";

            if (const auto Filebuffer = FS::Readfile("./Ayria/Clientinfo.json"); !Filebuffer.empty())
            {
                try
                {
                    // Allow comments in the JSON file.
                    const auto Object = nlohmann::json::parse(Filebuffer.c_str(), nullptr, true, true);
                    Localclient.ClientID = Object.value("ClientID", Localclient.ClientID);
                    Localclient.Username = Object.value("Username", Localclient.Username);
                    Localclient.Locale = Object.value("Locale", Localclient.Locale);
                }
                catch (const std::exception& e)
                {
                    Infoprint(va("Invalid Clientinfo.json: %s", e.what()));
                }
            }

            // Default group.
            Auxiliary::Joinmessagegroup();
            Auxiliary::Registermessagehandler(Hash::FNV1_32("Clientdiscovery"), Discoveryhandler);
        }
    }

    namespace API
    {
        static std::string JSON;
        extern "C" EXPORT_ATTR void __cdecl getLocalclient(const char **JSONString)
        {
            auto Object = nlohmann::json::object();
            Object["Locale"] = Localclient.Locale;
            Object["ClientID"] = Localclient.ClientID;
            Object["Username"] = Localclient.Username;

            JSON = Object.dump(4);
            *JSONString = JSON.c_str();
        }
    }
}

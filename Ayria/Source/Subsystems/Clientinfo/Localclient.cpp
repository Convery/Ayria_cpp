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
            const auto String = API::getLocalclient();
            Auxiliary::Sendmessage(Hash::FNV1_32("Clientdiscovery"), String);
        }
        void __cdecl Discoveryhandler(const char *JSONString)
        {
            Ayriaclient Newclient{};
            const auto Object = ParseJSON(JSONString);
            Newclient.ClientID = Object.value("ClientID", uint32_t());
            Newclient.Locale = Object.value("Locale", std::wstring());
            Newclient.Username = Object.value("Username", std::wstring());
            if (Newclient.ClientID && !Newclient.Username.empty()) Localnetwork.insert(std::move(Newclient));
        }

        static uint32_t Lastupdate{};
        void UpdateLocalclient()
        {
            const auto Currenttime = GetTickCount();
            if (Currenttime > (Lastupdate + 5000))
            {
                Lastupdate = Currenttime;

                // Announce our presence.
                Sendclientinfo();
            }
        }
        void InitLocalclient()
        {
            Localclient.ClientID = 0xDEADC0DE;
            Localclient.Username = L"Ayria";
            Localclient.Locale = L"English";

            if (const auto Filebuffer = FS::Readfile("./Ayria/Clientinfo.json"); !Filebuffer.empty())
            {
                const auto Object = ParseJSON(B2S(Filebuffer));
                Localclient.ClientID = Object.value("ClientID", Localclient.ClientID);
                Localclient.Username = Object.value("Username", Localclient.Username);
                Localclient.Locale = Object.value("Locale", Localclient.Locale);
            }

            // Default group.
            Auxiliary::Joinmessagegroup();
            Auxiliary::Registermessagehandler(Hash::FNV1_32("Clientdiscovery"), Discoveryhandler);
        }
    }

    namespace API
    {
        static std::string JSON;
        extern "C" EXPORT_ATTR const char *__cdecl getLocalclient()
        {
            auto Object = nlohmann::json::object();
            Object["Locale"] = Localclient.Locale;
            Object["ClientID"] = Localclient.ClientID;
            Object["Username"] = Localclient.Username;

            JSON = Object.dump(4);
            return JSON.c_str();
        }
    }
}

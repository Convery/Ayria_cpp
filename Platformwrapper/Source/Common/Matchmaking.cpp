


#include <Stdinclude.hpp>
#include <Common.hpp>
#include "../Steam/Steam.hpp"

namespace Matchmaking
{
    std::vector<Session_t> Netservers;
    static uint32_t Lastlocalupdate{};
    static uint32_t Lastnetupdate{};
    Session_t Localsession;
    bool isDirty{};

    std::vector<Session_t> *getNetworkservers()
    {
        const auto Currenttime = GetTickCount();
        if ((Currenttime - Lastnetupdate) > 2000)
        {
            if (const auto Callback = Ayria.API_Matchmake)
            {
                Netservers.clear();
                const auto Array = ParseJSON(Callback(Ayria.toFunctionID("getNetworksessions"), nullptr));
                for (const auto &Config : Array)
                {
                    Session_t Session{};
                    Session.Sessiondata = Config.value("Sessiondata", nlohmann::json::object());
                    Session.Playerdata = Config.value("Playerdata", nlohmann::json::object());
                    Session.Gameinfo = Config.value("Gameinfo", nlohmann::json::object());
                    Session.Hostinfo = Config.value("Hostinfo", nlohmann::json::object());
                    Session.HostID = Config.value("HostID", 0);
                    Netservers.push_back(Session);
                }
            }

            Lastnetupdate = Currenttime;
        }

        return &Netservers;
    }
    Session_t *getLocalsession()
    {
        // Ensure that we don't update in the middle of a frame.
        if (isDirty) [[likely]] return &Localsession;

        const auto Currenttime = GetTickCount();
        if ((Currenttime - Lastlocalupdate) > 2000)
        {
            if (const auto Callback = Ayria.API_Matchmake)
            {
                const auto Config = ParseJSON(Callback(Ayria.toFunctionID("getLocalsession"), nullptr));
                Localsession.Playerdata = Config.value("Playerdata", nlohmann::json::object());
                Localsession.Sessiondata = Config.value("Sessiondata", nlohmann::json::object());
                Localsession.Gameinfo = Config.value("Gameinfo", nlohmann::json::object());
                Localsession.Hostinfo = Config.value("Hostinfo", nlohmann::json::object());
                Localsession.HostID = Config.value("HostID", 0);
            }

            Lastlocalupdate = Currenttime;
        }

        return &Localsession;
    }

    void doFrame()
    {
        if (!isDirty) [[likely]] return;

        auto Object = nlohmann::json::object();
        Object["Hostinfo"] = Localsession.Hostinfo;
        Object["Gameinfo"] = Localsession.Gameinfo;
        Object["Playerdata"] = Localsession.Playerdata;
        Object["Sessiondata"] = Localsession.Sessiondata;

        const auto Plaintext = Object.dump();
        if (const auto Callback = Ayria.API_Matchmake)
        {
            Callback(Ayria.toFunctionID("Sessionupdate"), Object.dump().c_str());
        }

        isDirty = false;
    }
    void Update()
    {
        isDirty = true;
    }
}



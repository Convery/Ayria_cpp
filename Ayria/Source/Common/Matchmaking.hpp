/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-10-17
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>

namespace Matchmaking
{
    struct Session_t
    {
        // Internal use.
        bool isActive;
        uint32_t Lastmessage;

        Account_t Hostinfo;
        std::string JSONData;
        std::string Signature;
    };

    // Manage the sessions we know of, updates in the background.
    std::vector<Session_t *> getLANSessions();
    std::vector<Session_t *> getWANSessions();
    Session_t *getLocalsession();

    // Register handlers and set up session.
    void Initialize();

    // Add API handlers.
    inline std::string __cdecl getSessions(const char *JSONString)
    {
        const auto Request = ParseJSON(JSONString);
        const auto noLAN = Request.value("noLAN", false);
        const auto noWAN = Request.value("noWAN", false);
        const auto noSelf = Request.value("noSelf", false);

        auto Object = nlohmann::json::object();
        if (!noLAN)
        {
            Object["LAN"] = nlohmann::json::array();
            for (const auto &Session : getLANSessions())
            {
                Object["LAN"] += nlohmann::json::object({
                    { "Hostlocale", Session->Hostinfo.Locale.asUTF8() },
                    { "Hostname", Session->Hostinfo.Username.asUTF8() },
                    { "HostID", Session->Hostinfo.ID.Raw },
                    { "Sessiondata", Session->JSONData }
                });
            }
        }
        if (!noWAN)
        {
            Object["WAN"] = nlohmann::json::array();
            for (const auto &Session : getWANSessions())
            {
                Object["WAN"] += nlohmann::json::object({
                    { "Hostlocale", Session->Hostinfo.Locale.asUTF8() },
                    { "Hostname", Session->Hostinfo.Username.asUTF8() },
                    { "HostID", Session->Hostinfo.ID.Raw },
                    { "Sessiondata", Session->JSONData }
                });
            }
        }
        if (!noSelf)
        {
            const auto Session = getLocalsession();
            if (Session)
            {
                Object["Localsession"] = nlohmann::json::object({
                    { "Hostlocale", Session->Hostinfo.Locale.asUTF8() },
                    { "Hostname", Session->Hostinfo.Username.asUTF8() },
                    { "HostID", Session->Hostinfo.ID.Raw },
                    { "Sessiondata", Session->JSONData }
                });
            }
        }

        return DumpJSON(Object);
    }
    inline std::string __cdecl updateSession(const char *JSONString)
    {
        const auto Client = Clientinfo::getLocalclient();
        auto Session = getLocalsession();
        Session->isActive = true;

        // Ensure that the account-type is OK.
        Session->Hostinfo = *Client;
        Session->Hostinfo.ID.Accounttype.isServer = true;

        // Update the existing session.
        auto Sessiondata = ParseJSON(Session->JSONData);
        Sessiondata.update(ParseJSON(JSONString));
        Session->JSONData = DumpJSON(Sessiondata);

        // Sign so that others can verify the data.
        Session->Signature = PK_RSA::Signmessage(Session->JSONData, Clientinfo::getSessionkey());

        // Nothing returned, use getSessions for latest info.
        return "{}";
    }
    inline std::string __cdecl terminateSession(const char *)
    {
        auto Session = getLocalsession();
        Session->isActive = false;
        return "{}";
    }
    inline void API_Initialize()
    {
        Initialize();

        API::Registerhandler_Matchmake("getSessions", getSessions);
        API::Registerhandler_Matchmake("updateSession", updateSession);
        API::Registerhandler_Matchmake("terminateSession", terminateSession);
    }
}

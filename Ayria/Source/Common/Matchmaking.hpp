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
        const auto Request = JSON::Parse(JSONString);
        const auto noLAN = Request.value("noLAN", false);
        const auto noWAN = Request.value("noWAN", false);
        const auto noSelf = Request.value("noSelf", false);

        JSON::Object_t Object;
        if (!noLAN)
        {
            for (const auto &Session : getLANSessions())
            {
                Object["LAN"].get<JSON::Array_t>().push_back(JSON::Object_t({
                    { "Hostlocale", Encoding::toNarrow(Session->Hostinfo.Locale) },
                    { "Hostname", Encoding::toNarrow(Session->Hostinfo.Username) },
                    { "HostID", Session->Hostinfo.ID.Raw },
                    { "Sessiondata", Session->JSONData }
                }));
            }
        }
        if (!noWAN)
        {
            for (const auto &Session : getWANSessions())
            {
                Object["WAN"].get<JSON::Array_t>().push_back(JSON::Object_t({
                    { "Hostlocale", Encoding::toNarrow(Session->Hostinfo.Locale) },
                    { "Hostname", Encoding::toNarrow(Session->Hostinfo.Username) },
                    { "HostID", Session->Hostinfo.ID.Raw },
                    { "Sessiondata", Session->JSONData }
                }));
            }
        }
        if (!noSelf)
        {
            const auto Session = getLocalsession();
            if (Session)
            {
                Object["Localsession"] = JSON::Object_t({
                    { "Hostlocale", Encoding::toNarrow(Session->Hostinfo.Locale) },
                        { "Hostname", Encoding::toNarrow(Session->Hostinfo.Username) },
                    { "HostID", Session->Hostinfo.ID.Raw },
                    { "Sessiondata", Session->JSONData }
                });
            }
        }

        return JSON::Dump(Object);
    }
    inline std::string __cdecl updateSession(const char *JSONString)
    {
        const auto Client = Clientinfo::getLocalclient();
        auto Session = getLocalsession();

        // Ensure that the host is initialized.
        Session->Hostinfo = *Client;
        Session->isActive = true;

        // Update the existing session.
        auto Sessiondata = JSON::Parse(Session->JSONData);
        Sessiondata.update(JSON::Parse(JSONString));
        Session->JSONData = JSON::Dump(Sessiondata);

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

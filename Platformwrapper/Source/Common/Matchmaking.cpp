/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-10-27
    License: MIT
*/

#include <Stdinclude.hpp>
#include "../Common.hpp"

namespace Matchmaking
{
    std::vector<Session_t> LANSessions, WANSessions;
    uint64_t Lasthash{}, Lastupdate{};
    std::atomic_flag isDirty{};
    Session_t Localsession{};

    // Manage the sessions we know of, updates in the background.
    std::vector<Session_t *> getLANSessions()
    {
        std::vector<Session_t *> Result;
        Result.reserve(LANSessions.size());
        for (auto it = LANSessions.begin(); it != LANSessions.end(); ++it)
            Result.push_back(&*it);
        return Result;
    }
    std::vector<Session_t *> getWANSessions()
    {
        std::vector<Session_t *> Result;
        Result.reserve(WANSessions.size());
        for (auto it = WANSessions.begin(); it != WANSessions.end(); ++it)
            Result.push_back(&*it);
        return Result;
    }
    Session_t *getLocalsession()
    {
        return &Localsession;
    }

    // Notify the backend about our session changing.
    void Invalidatesession()
    {
        isDirty.test_and_set();
    }
    void doFrame()
    {
        if (isDirty.test())
        {
            const auto String{ DumpJSON(Localsession.toJSON()) };
            const auto Currenthash{ Hash::FNV1_32(String) };
            if (Currenthash != Lasthash)
            {
                if (const auto Callback = Ayria.API_Matchmake) [[likely]]
                {
                    Callback(Ayria.toFunctionID("updateSession"), String.c_str());
                }

                Lasthash = Currenthash;
            }

            isDirty.clear();
        }

        const auto Currentclock{ GetTickCount64() };
        if (Currentclock - Lastupdate > 5000)
        {
            if (const auto Callback = Ayria.API_Matchmake) [[likely]]
            {
                const auto Result = Callback(Ayria.toFunctionID("getSessions"), nullptr);
                const auto Object = ParseJSON(Result);

                if (Object.contains("LAN"))
                {
                    LANSessions.clear();
                    for (const auto &Session : Object["LAN"])
                        LANSessions.push_back(Session_t(Session));
                }
                if (Object.contains("WAN"))
                {
                    WANSessions.clear();
                    for (const auto &Session : Object["WAN"])
                        WANSessions.push_back(Session_t(Session));
                }
                if (Object.contains("Localsession"))
                {
                    Localsession = Session_t(Object["Localsession"]);
                }
            }

            Lastupdate = Currentclock;
        }
    }
}

/*
    Initial author: Convery (tcn@ayria.se)
    Started: 05-06-2019
    License: MIT
*/

#if !defined(_WIN32)
    #error Not currently available on NIX!
#endif
#include "../Steam.hpp"
#include <Objbase.h>

// GUID serialization.
void to_json(nlohmann::json &JSON, const GUID &ID)
{
    JSON = va("%lu-%hu-%hu-%llu", ID.Data1, ID.Data2, ID.Data3, *(uint64_t *)ID.Data4);
}
void from_json(const nlohmann::json &JSON, GUID &ID)
{
    std::sscanf(JSON.get<std::string>().c_str(), "%lu-%hu-%hu-%llu", &ID.Data1, &ID.Data2, &ID.Data3, (uint64_t *)&ID.Data4);
}

namespace Steam
{
    namespace Matchmaking
    {
        std::vector<Server_t> Knownservers;
        Server_t Localserver{};
        GUID SessionID{};

        void Createsession()
        {
            // Redundant call.
            if (SessionID.Data1) return;

            // If this fails, we have bigger issues than proper randomness.
            if (CoCreateGuid(&SessionID))
            {
                srand(time(NULL) & 0xFFFFEFFF);
                for (int i = 0; i < 4; ++i) ((uint32_t *)&SessionID)[i] = rand();
                for (int i = 1; i < 15; i = rand() % 16 + 1) ((uint8_t *)&SessionID)[i] ^= ((uint8_t *)&SessionID)[i - 1];
            }

            std::thread([]()
            {
                // Quit if it's invalidated.
                while(SessionID.Data1)
                {
                    const uint64_t Currenttime = time(NULL);

                    // Process any new requests, we currently only get the "Announce" subtype so no need to verify.
                    for (const auto &Item : Global.Synchronisation->getResponses("Matchmaking"))
                    {
                        try
                        {
                            // Parse the payload-data as JSON, for some reason std::string doesn't decay.
                            auto Object = nlohmann::json::parse(Base64::Decode(Item.Data).c_str());
                            if (Object["SessionID"].is_null()) continue;

                            // We receive our own sessions update as well, to help developers.
                            if (SessionID == Object["SessionID"].get<GUID>()) continue;

                            // Find the server to update or create a new one.
                            auto Server = std::find_if(Knownservers.begin(), Knownservers.end(), [&](const auto A) { return Object["SessionID"] == A.Gamedata["SessionID"]; });
                            if (Server == Knownservers.end())
                            {
                                Knownservers.emplace_back().Hostaddress = Item.Sender;
                                Server = Knownservers.end()--;
                            }

                            // Highly advanced protection against fuckery here..
                            if (Server->Hostaddress != Item.Sender) continue;

                            // Just mindlessly overwrite the servers info.
                            for (const auto &[Key, Value] : Object.items())
                            {
                                Server->Gamedata.emplace(Key, Value);
                            }

                            // We should get an update every 30 sec =)
                            Server->Lastpingtime = time(NULL);
                        }
                        catch (std::exception &e) { (void)e; }
                    }

                    // Announce an update if needed.
                    if (Currenttime - Localserver.Lastpingtime > 30)
                        Updateserver();

                    LOOP: // Cleanup of outdated servers.
                    for(auto Iterator = Knownservers.begin(); Iterator != Knownservers.end(); ++Iterator)
                    {
                        if(Currenttime - Iterator->Lastpingtime > 40)
                        {
                            Knownservers.erase(Iterator);
                            goto LOOP;
                        }
                    }

                    // Sleep until the next update.
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
            }).detach();
        }
        void Updateserver()
        {
            // Are we even hosting anything? =P
            if(!Localserver.Hostaddress.empty())
            {
                Localserver.Gamedata["SessionID"] = SessionID;
                Global.Synchronisation->Broadcast("Matchmaking", "Announce", Localserver.Gamedata.dump());
            }

            Localserver.Lastpingtime = time(NULL);
        }
    }
}

/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-11-22
    License: MIT
*/

#include <Global.hpp>

namespace Services::Clientrelations
{
    // Layer 2 interaction.
    namespace Messagehandlers
    {
        static bool __cdecl onInsert(uint64_t, const char *LongID, const char *Message, unsigned int Length)
        {
            const auto Parsed = JSON::Parse(Message, Length);
            const auto isFriend = Parsed.value<bool>("isFriend");
            const auto isBlocked = Parsed.value<bool>("isBlocked");
            const auto ClientID = Parsed.value<std::string>("ClientID");

            // Sanity-checking.
            if (ClientID.empty()) [[unlikely]] return false;
            if (!(isFriend ^ isBlocked)) [[unlikely]] return false;

            // Check if we should care for this event.
            do
            {
                // Not interesting to us.
                if (ClientID != Global.getLongID()) break;

                // We are now mutual friends.
                if (isFriend && AyriaAPI::Clientrelations::Get(Global.getLongID(), ClientID).first)
                {
                    Layer4::Publish("Relation::onFriendship", JSON::Object_t{ {"ClientID", ClientID} });
                    break;
                }

                // They asked to be our friend.
                if (isFriend)
                {
                    Layer4::Publish("Relation::onFriendrequest", JSON::Object_t{ {"ClientID", ClientID} });
                    break;
                }

                // NOTE(tcn): Not sure if we should publish this change. It's not secret, but maybe unnecessary.
                Layer4::Publish("Relation::onBlocked", JSON::Object_t{ {"ClientID", ClientID} });

            } while (false);

            // Update the database.
            Backend::Database()
                << "INSERT OR REPLACE INTO Clientrelation VALUES (?, ?, ?, ?);"
                << LongID << ClientID << isBlocked << isFriend;

            return true;
        }
        static bool __cdecl onErase(uint64_t, const char *LongID, const char *Message, unsigned int Length)
        {
            const auto ClientID = JSON::Parse(Message, Length).value<std::string>("ClientID");
            if (ClientID.empty()) [[unlikely]] return false;

            // Check if we should care for this event.
            do
            {
                // Not interesting to us.
                if (ClientID != Global.getLongID()) break;

                const auto Relation = AyriaAPI::Clientrelations::Get(LongID, ClientID);
                if (Relation.first)
                {
                    Layer4::Publish("Relation::unfriended", JSON::Object_t{ {"ClientID", ClientID} });
                    break;
                }
                if (Relation.second)
                {
                    Layer4::Publish("Relation::unblocked", JSON::Object_t{ {"ClientID", ClientID} });
                    break;
                }

            } while (false);

            // Remove all relevant entries.
            Backend::Database()
                << "DELETE FROM Clientrelation WHERE (Source = ? AND Target = ?);"
                << LongID << ClientID;

            return true;
        }
    }

    // Layer 3 interaction.
    namespace JSONAPI
    {
        static std::string Wrapper(JSON::Value_t &&Request, bool isFriend, bool isBlocked)
        {
            // Not sure what the caller expects of us..
            if (!Request.contains("ClientID")) [[unlikely]]
                return R"({ "Error" : "No client ID provided." })";

            // Validate that the client is in the DB.
            if (!Clientinfo::getClient(Request.value<std::string>("ClientID"))) [[unlikely]]
                return R"({ "Error" : "Client ID is not in the DB (yet?)." })";

            // Request to clear.
            if (!isFriend && !isBlocked)
            {
                // Remove all relevant entries.
                Backend::Database()
                    << "DELETE FROM Clientrelation WHERE (Source = ? AND Target = ?);"
                    << Global.getLongID() << Request["ClientID"].get<std::string>();

                Layer1::Publish("Clientrelations::Erase", JSON::Object_t({ { "ClientID", Request["ClientID"] } }));
                return {};
            }

            // Update the database.
            Backend::Database()
                << "INSERT OR REPLACE INTO Clientrelation VALUES (?, ?, ?, ?);"
                << Global.getLongID() << Request["ClientID"].get<std::string>() << isBlocked << isFriend;

            // Filter out unneccessary properties.
            const auto Object = JSON::Object_t({
                { "ClientID", Request["ClientID"] },
                { "isBlocked", isBlocked },
                { "isFriend", isFriend }
            });
            Layer1::Publish("Clientrelations::Insert", Object);
            return {};
        }
        static std::string __cdecl Clear(JSON::Value_t &&Request)
        {
            return Wrapper(std::move(Request), false, false);
        }
        static std::string __cdecl Block(JSON::Value_t &&Request)
        {
            return Wrapper(std::move(Request), false, true);
        }
        static std::string __cdecl Befriend(JSON::Value_t &&Request)
        {
            return Wrapper(std::move(Request), true, false);
        }
    }

    // Add the handlers and tasks.
    void Initialize()
    {
        Backend::Database() <<
            "CREATE TABLE IF NOT EXISTS Relation ("
            "Source TEXT REFERENCES Account(Publickey) ON DELETE CASCADE, "
            "Target TEXT REFERENCES Account(Publickey) ON DELETE CASCADE, "
            "isBlocked BOOLEAN NOT NULL, "
            "isFriend BOOLEAN NOT NULL, "
            "UNIQUE (Source, Target) );";

        // Parse Layer 2 messages.
        Layer2::addMessagehandler("Clientrelations::Insert", Messagehandlers::onInsert);
        Layer2::addMessagehandler("Clientrelations::Erase", Messagehandlers::onErase);

        // Accept Layer 3 calls.
        Layer3::addEndpoint("Relation::Clear", JSONAPI::Clear);
        Layer3::addEndpoint("Relation::Block", JSONAPI::Block);
        Layer3::addEndpoint("Relation::Befriend", JSONAPI::Befriend);
    }
}

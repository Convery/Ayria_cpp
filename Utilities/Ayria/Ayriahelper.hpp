/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2022-01-10
    License: MIT

    Helpers for common operations.
*/

#pragma once
#include <Stdinclude.hpp>

namespace AyriaJSON
{
    using LongID_t = std::string;

    namespace Relations
    {
        inline bool addFriend(const LongID_t &ClientID)
        {
            const auto Request = JSON::Object_t{ { "ClientID", ClientID } };
            const auto Response = Ayria.doRequest("Clientrelations::Befriend", Request);

            return Response.contains("Error");
        }
        inline bool removeFriend(const LongID_t &ClientID)
        {
            const auto Request = JSON::Object_t{ { "ClientID", ClientID } };
            const auto Response = Ayria.doRequest("Clientrelations::Clear", Request);

            return Response.contains("Error");
        }

        inline bool blockUser(const LongID_t &ClientID)
        {
            const auto Request = JSON::Object_t{ { "ClientID", ClientID } };
            const auto Response = Ayria.doRequest("Clientrelations::Block", Request);

            return Response.contains("Error");
        }
        inline bool unblockUser(const LongID_t &ClientID)
        {
            return removeFriend(ClientID);
        }
    }

    namespace Groups
    {
        // Creation and updates are handled the same in the backend, so alias is provided for clarity.
        inline std::optional<std::string> Creategroup(std::optional<std::u8string> Groupname, uint32_t Wantedusers = 0)
        {
            const auto Request = JSON::Object_t{ { "Groupname", Groupname }, { "Wantedusers", Wantedusers } };
            const auto Response = Ayria.doRequest("Groups::Updategroup", Request);

            if (Response.contains("Error")) return { Response["Error"].get<std::string>() };
            else return {};
        }
        inline std::optional<std::string> Updategroup(std::optional<std::u8string> Groupname, uint32_t Wantedusers = 0)
        {
            return Creategroup(Groupname, Wantedusers);
        }

        // If the host leaves the group, it gets destroyed.
        inline void Leavegroup(const LongID_t &GroupID)
        {
            const auto Request = JSON::Object_t{ { "GroupID", GroupID } };
            Ayria.doRequest("Group::Leave", Request);
        }

        // Paired with getJoinrequests from AyriaAPI
        inline bool Requestjoin(const LongID_t &GroupID)
        {
            const auto Request = JSON::Object_t{ { "GroupID", GroupID } };
            const auto Response = Ayria.doRequest("Group::Requestjoin", Request);

            return Response.contains("Error");
        }
        inline void Cancelrequest(const LongID_t &GroupID)
        {
            return Leavegroup(GroupID);
        }

        // User management.
        inline std::optional<std::string> addClient(const LongID_t &ClientID, std::optional<LongID_t> GroupID)
        {
            const auto Request = JSON::Object_t{ { "ClientID", ClientID }, { "GroupID", GroupID } };
            const auto Response = Ayria.doRequest("Group::addClient", Request);

            if (Response.contains("Error")) return { Response["Error"].get<std::string>() };
            else return {};
        }
        inline std::optional<std::string> kickClient(const LongID_t &ClientID, std::optional<LongID_t> GroupID)
        {
            const auto Request = JSON::Object_t{ { "ClientID", ClientID }, { "GroupID", GroupID } };
            const auto Response = Ayria.doRequest("Group::kickClient", Request);

            if (Response.contains("Error")) return { Response["Error"].get<std::string>() };
            else return {};
        }

        // Permissions management.
        inline std::optional<std::string> promoteClient(const LongID_t &ClientID, std::optional<LongID_t> GroupID)
        {
            const auto Request = JSON::Object_t{ { "ClientID", ClientID }, { "GroupID", GroupID } };
            const auto Response = Ayria.doRequest("Group::promoteClient", Request);

            if (Response.contains("Error")) return { Response["Error"].get<std::string>() };
            else return {};
        }
        inline std::optional<std::string> demoteClient(const LongID_t &ClientID, std::optional<LongID_t> GroupID)
        {
            const auto Request = JSON::Object_t{ { "ClientID", ClientID }, { "GroupID", GroupID } };
            const auto Response = Ayria.doRequest("Group::demoteClient", Request);

            if (Response.contains("Error")) return { Response["Error"].get<std::string>() };
            else return {};
        }

        // Update the groups cryptography (useful after kicking).
        inline bool invalidateCryptokey(const LongID_t &GroupID)
        {
            const auto Request = JSON::Object_t{ { "GroupID", GroupID } };
            const auto Response = Ayria.doRequest("Group::reKey", Request);

            return Response.contains("Error");
        }



    }

    namespace Matchmaking
    {
        // Startup and shutdown.
        inline std::optional<std::string> Createserver(const LongID_t &GroupID, std::string Hostaddress)
        {
            const auto Request = JSON::Object_t{ { "GroupID", GroupID }, { "Hostaddress", Hostaddress } };
            const auto Response = Ayria.doRequest("Matchmaking::Createserver", Request);

            if (Response.contains("Error")) return { Response["Error"].get<std::string>() };
            else return {};
        }
        inline void Terminateserver()
        {
            Ayria.doRequest("Matchmaking::Terminateserver", {});
        }

        // Where other clients can look for more info, e.g. "Steam".
        inline void addProvider(const LongID_t &GroupID, uint32_t ProviderID)
        {
            const auto Request = JSON::Object_t{ { "GroupID", GroupID }, { "ProviderID", ProviderID } };
            Ayria.doRequest("Matchmaking::addProvider", Request);
        }
        inline void removeProvider(const LongID_t &GroupID, uint32_t ProviderID)
        {
            const auto Request = JSON::Object_t{ { "GroupID", GroupID }, { "ProviderID", ProviderID } };
            Ayria.doRequest("Matchmaking::removeProvider", Request);
        }
        inline void addProvider(const LongID_t &GroupID, std::u8string_view Provider)
        {
            return addProvider(GroupID, Hash::WW32(Provider));
        }
        inline void removeProvider(const LongID_t &GroupID, std::u8string_view Provider)
        {
            return addProvider(GroupID, Hash::WW32(Provider));
        }



        // Make server available
        // Handle join requests -> Group::Handle joins



    }
}

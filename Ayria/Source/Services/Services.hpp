/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-12-17
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>

struct Client_t
{
    uint32_t UserID;
    const std::u8string *Username;
};

// Helper for async tasks.
template<typename T> struct Task_t
{
    std::atomic_flag hasResult, isDone;
    T Result;
};

namespace Clientinfo
{
    // If another clients crypto-key is needed, we request it.
    void Requestcryptokeys(std::vector<uint32_t> UserIDs);
    std::string getCryptokey(uint32_t UserID);

    // Fetch information about a client, local or remote.
    bool isClientauthenticated(uint32_t UserID);
    Client_t *getWANClient(uint32_t UserID);
    Client_t *getLANClient(uint32_t NodeID);
    bool isClientonline(uint32_t UserID);

    // Primarily used for cryptography.
    std::string_view getHardwareID();

    //
    void Initializecrypto();
    void Initializediscovery();
    inline void Initialize()
    {
        Initializediscovery();
        Initializecrypto();
    }
}

namespace Social
{
    // View this as a simple friends-list for now.
    namespace Relations
    {
        // Fetch information about clients.
        bool isBlocked(uint32_t UserID);

        // Load the relations from disk.
        void Initialize();
    }

    // A collection of active users.
    namespace Groups
    {
        // Verify membership.
        bool isMember(uint64_t GroupID, uint32_t UserID);
        bool isAdmin(uint64_t GroupID, uint32_t UserID);

        // Set up message-handlers.
        void Initialize();
    }

    // Simple P2P messaging.
    namespace Messages
    {
        // Add the message-handlers and load message history from disk.
        void Initialize();
    }

    // Simple key-value stored shared with clients.
    namespace Presence
    {
        void Initialize();
    }


    //
    inline void Initialize()
    {
        Groups::Initialize();
        Messages::Initialize();
        Presence::Initialize();
        Relations::Initialize();
    }
}

namespace Matchmaking
{
    // Currently hosting.
    bool isActive();

    // Set up handlers.
    void Initialize();
}

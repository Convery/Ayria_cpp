/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-12-17
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>

namespace Services
{
    // Helpers for database access.
    namespace DB
    {
        // Prepare a query for the DB. Creates and initialize it if needed.
        sqlite::database Query();

        // Get the primary key for a client.
        uint32_t getKeyhash(uint32_t AccountID);

        // Set the last-modified timestamp for a client.
        inline void Touchclient(uint32_t Keyhash)
        {
            try
            {
                Query() << "REPLACE INTO Knownclients (Timestamp) VALUES (?) WHERE Sharedkeyhash = ?;"
                        << time(NULL) << Keyhash;
            } catch (...) {}
        }
        inline void Timeoutclient(uint32_t Keyhash)
        {
            try
            {
                Query() << "REPLACE INTO Knownclients (Timestamp) VALUES (?) WHERE Sharedkeyhash = ?;"
                        << 0 << Keyhash;
            } catch (...) {}
        }

        // Helpers to get the tables.
        inline JSON::Object_t getKnownclients(uint32_t Keyhash)
        {
            JSON::Object_t Result{};
            try
            {
                Query() << "SELECT * FROM Knownclients WHERE Sharedkeyhash = ?;" << Keyhash
                        >> [&](uint32_t Sharedkeyhash, uint32_t AccountID, const std::string &Username, bool Validated, uint32_t Timestamp)
                        {
                            Result = JSON::Object_t({
                                { "Sharedkeyhash", Sharedkeyhash },
                                { "AccountID", AccountID },
                                { "Username", Username },
                                { "Validated", Validated },
                                { "Timestamp", Timestamp }
                            });
                        };
            } catch (...) {}

            return Result;
        }
        inline JSON::Object_t getClientinfo(uint32_t Keyhash)
        {
            JSON::Object_t Result{};
            try
            {
                Query() << "SELECT * FROM Clientinfo WHERE AyriaID = ?;" << Keyhash
                        >> [&](uint32_t AyriaID, const std::string &Sharedkey, uint32_t InternalIP, uint32_t ExternalIP, uint32_t GameID, uint32_t ModID)
                        {
                            Result = JSON::Object_t({
                                { "AyriaID", AyriaID },
                                { "Sharedkey", Sharedkey },
                                { "InternalIP", InternalIP },
                                { "ExternalIP", ExternalIP },
                                { "GameID", GameID },
                                { "ModID", ModID }
                            });
                        };
            } catch (...) {}

            return Result;
        }
        inline JSON::Array_t getClientpresence(uint32_t Keyhash)
        {
            JSON::Array_t Result{};
            try
            {
                Query() << "SELECT * FROM Clientpresence WHERE AyriaID = ?;" << Keyhash
                        >> [&](uint32_t AyriaID, const std::string &Key, const std::string &Value)
                        {
                            Result.emplace_back(JSON::Object_t({
                                { "AyriaID", AyriaID },
                                { "Value", Value },
                                { "Key", Key }
                            }));
                        };
            } catch (...) {}

            return Result;
        }

        // Helpers to set the tables.
        inline bool setKnownclients(JSON::Value_t &&Keyvalues)
        {
            // Without a primary key we can't do much.
            if (!Keyvalues.contains("Sharedkeyhash")) return false;

            const auto Sharedkeyhash = Keyvalues.value<uint32_t>("Sharedkeyhash");
            const auto Username = Keyvalues.value<std::string>("Username");
            const auto AccountID = Keyvalues.value<uint32_t>("AccountID");
            const auto Validated = Keyvalues.value<bool>("Validated");
            const auto Timestamp = time(NULL);

            // Can we do a full insert or do we need to update each value?
            if (!Keyvalues.contains("AccountID") || !Keyvalues.contains("Username") || !Keyvalues.contains("Validated"))
            {
                try
                {
                    if (Keyvalues.contains("AccountID"))
                    {
                        Query() << "INSERT OR REPLACE INTO Knownclients (Sharedkeyhash, AccountID, Timestamp) VALUES (?, ?, ?);"
                                << Sharedkeyhash << AccountID << Timestamp;
                    }
                    if (Keyvalues.contains("Username"))
                    {
                        Query() << "INSERT OR REPLACE INTO Knownclients (Sharedkeyhash, Username, Timestamp) VALUES (?, ?, ?);"
                                << Sharedkeyhash << Username << Timestamp;
                    }
                    if (Keyvalues.contains("Validated"))
                    {
                        Query() << "INSERT OR REPLACE INTO Knownclients (Sharedkeyhash, Validated, Timestamp) VALUES (?, ?, ?);"
                                << Sharedkeyhash << Validated << Timestamp;
                    }

                    return true;
                }
                catch (...) {}
            }
            else
            {
                try
                {
                    Query()
                        << "INSERT OR REPLACE INTO Knownclients (Sharedkeyhash, AccountID, Username, Validated, Timestamp) VALUES (?, ?, ?, ?, ?);"
                        << Sharedkeyhash << AccountID << Username << Validated << Timestamp;
                    return true;
                }
                catch (...) {}
            }

            return false;
        }
        inline bool setClientinfo(JSON::Value_t &&Keyvalues)
        {
            // Without a primary key we can't do much.
            if (!Keyvalues.contains("AyriaID")) return false;

            const auto Sharedkey = Keyvalues.value<std::string>("Sharedkey");
            const auto InternalIP = Keyvalues.value<uint32_t>("InternalIP");
            const auto ExternalIP = Keyvalues.value<uint32_t>("ExternalIP");
            const auto AyriaID = Keyvalues.value<uint32_t>("AyriaID");
            const auto GameID = Keyvalues.value<uint32_t>("GameID");
            const auto ModID = Keyvalues.value<uint32_t>("ModID");

            // Can we do a full insert or do we need to update each value?
            if (!Keyvalues.contains("Sharedkey") || !Keyvalues.contains("InternalIP") || !Keyvalues.contains("ExternalIP") ||
                !Keyvalues.contains("GameID") || !Keyvalues.contains("ModID"))
            {
                try
                {
                    if (Keyvalues.contains("Sharedkey"))
                    {
                        Query() << "INSERT OR REPLACE INTO Clientinfo (AyriaID, Sharedkey) VALUES (?, ?);"
                                << AyriaID << Sharedkey;
                    }
                    if (Keyvalues.contains("InternalIP"))
                    {
                        Query() << "INSERT OR REPLACE INTO Clientinfo (AyriaID, InternalIP) VALUES (?, ?);"
                                << AyriaID << InternalIP;
                    }
                    if (Keyvalues.contains("ExternalIP"))
                    {
                        Query() << "INSERT OR REPLACE INTO Clientinfo (AyriaID, ExternalIP) VALUES (?, ?);"
                                << AyriaID << ExternalIP;
                    }
                    if (Keyvalues.contains("GameID"))
                    {
                        Query() << "INSERT OR REPLACE INTO Clientinfo (AyriaID, GameID) VALUES (?, ?);"
                                << AyriaID << GameID;
                    }
                    if (Keyvalues.contains("ModID"))
                    {
                        Query() << "INSERT OR REPLACE INTO Clientinfo (AyriaID, ModID) VALUES (?, ?);"
                                << AyriaID << ModID;
                    }

                    Touchclient(AyriaID);
                    return true;
                }
                catch (...) {}
            }
            else
            {
                try
                {
                    Query()
                        << "INSERT OR REPLACE INTO Clientinfo (AyriaID, Sharedkey, InternalIP, ExternalIP, GameID, ModID) VALUES (?, ?, ?, ?, ?, ?);"
                        << AyriaID << Sharedkey << InternalIP << ExternalIP << GameID << ModID;

                    Touchclient(AyriaID);
                    return true;
                }
                catch (...) {}
            }

            return false;
        }
        inline bool setClientpresence(JSON::Array_t &&Keyvalues)
        {
            try
            {
                for (const auto &Item : Keyvalues)
                {
                    const auto AyriaID = Item.value<uint32_t>("AyriaID");
                    const auto Value = Item.value<std::string>("Value");
                    const auto Key = Item.value<std::string>("Key");

                    Query() << "INSERT OR REPLACE INTO Clientpresence (AyriaID, Key, Value) VALUES (?, ?, ?);"
                            << AyriaID << Key << Value;
                }

                return true;
            }
            catch (...) {}

            return false;
        }
    }

    // Allow plugins to listen to events from the services.
    using Eventcallback_t = void(__cdecl *)(uint32_t Identifier, const char *Message, unsigned int Length);
    void Postevent(std::string_view Identifier, const char *Message, unsigned int Length);
    void Subscribetoevent(std::string_view  Identifier, Eventcallback_t Callback);
    void Unsubscribe(std::string_view  Identifier, Eventcallback_t Callback);

    void Postevent(uint32_t Identifier, const char *Message, unsigned int Length);
    void Subscribetoevent(uint32_t Identifier, Eventcallback_t Callback);
    void Unsubscribe(uint32_t Identifier, Eventcallback_t Callback);

    namespace Clientinfo
    {
        // Convert between AccountID and LAN SessionID.
        uint32_t getAccountID(uint32_t SessionID);
        uint32_t getSessionID(uint32_t AccountID);

        // Derive a key for encryption.
        std::string Derivecryptokey(uint32_t AccountID);

        // Create a proper key for server authentication, returns the email-hash.
        std::string Createkey(std::string_view Email, std::string_view Password);

        // Generate an identifier for the current hardware.
        std::string GenerateHWID();

        // Set up the service.
        void Initialize();
    }

    namespace Messaging
    {
        // Send an instant-message to clients.
        bool Sendusermessage(uint32_t AccountID, std::string_view Message, bool Encrypt = false);
        bool Sendgroupmessage(uint64_t GroupID, std::string_view Message);

        // Set up the service.
        void Initialize();
    }

    namespace Clientpresence
    {
        // Set up the service.
        void Initialize();
    }

    namespace Fileshare
    {
        struct Fileinfo_t
        {
            std::unordered_map<uint64_t, uint64_t> Filetags;
            std::string Directory;
            std::string Filename;
            uint32_t Checksum;
            uint32_t FileID;
            uint32_t Size;
        };

        // All paths are relative to the Ayria/Storage directory.
        const Fileinfo_t *addFile(const std::string &Relativepath, std::unordered_map<uint64_t, uint64_t> &&Tags);
        void remFile(const std::string &Relativepath);
        void remFile(uint32_t FileID);

        // Get a listing for another client or the ones we currently share.
        const std::vector<Fileinfo_t> *getListing(uint32_t AccountID);
        const std::vector<Fileinfo_t> &getCurrentlist();

        // Set up the service.
        void Initialize();
    }

    // Set up the services.
    inline void Initialize()
    {
        Clientinfo::Initialize();
        Messaging::Initialize();
        Clientpresence::Initialize();
    }
}











namespace Services
{
    union Grouptype_t
    {
        uint8_t Raw;
        struct
        {
            uint8_t
                isClan : 1,
                isPublic : 1,
                isGamelobby : 1,
                isLocalhost : 1,
                isChatgroup : 1,
                isFriendgroup : 1;
        };
    };
    union GroupID_t
    {
        uint64_t Raw;
        struct
        {
            uint32_t AdminID;   // Creator.
            uint16_t RoomID;    // Random.
            uint8_t Limit;      // Users.
            uint8_t Type;       // Server, localhost, or clan.
        };
    };

    namespace Clientinfo
    {
        // Helper functionallity.
        std::string_view getHardwareID();
        uint32_t getClientID(uint32_t NodeID);
        std::string getSharedkey(uint32_t ClientID);

        // Set up the service.
        void Initialize();
    }

    namespace Matchmakingsessions
    {
        // Set up the service.
        void Initialize();
    }

    namespace Relationships
    {
        // Helper functionallity.
        bool isBlocked(uint32_t SourceID, uint32_t TargetID);
        bool isFriend(uint32_t SourceID, uint32_t TargetID);

        // Set up the service.
        void Initialize();
    }

    namespace Usergroups
    {
        // Helper functionallity.
        bool isMember(uint64_t GroupID, uint32_t ClientID);

        // Set up the service.
        void Initialize();
    }

    namespace Messaging
    {
        // Set up the service.
        void Initialize();
    }

    namespace Presence
    {
        // Set up the service.
        void Initialize();
    }

    // Set up the services.
    inline void Initialize()
    {
        Presence::Initialize();
        Messaging::Initialize();
        Clientinfo::Initialize();
        Usergroups::Initialize();
        Relationships::Initialize();
        Matchmakingsessions::Initialize();
    }
}

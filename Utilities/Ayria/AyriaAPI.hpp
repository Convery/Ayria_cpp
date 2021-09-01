/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-04-19
    License: MIT

    Helpers for common database operations.
    Errors return default constructed results, see Ayria.log for more info.
*/

#if defined (__cplusplus)
#pragma once
#include <Stdinclude.hpp>
#include "Ayriamodule.h"

namespace AyriaAPI
{
    // For unfiltered access to the DB, use with care.
    inline sqlite::database Query()
    {
        static std::shared_ptr<sqlite3> Database{};
        if (!Database) [[unlikely]]
        {
            sqlite3 *Ptr{};

            // :memory: should never fail unless the client has more serious problems..
            const auto Result = sqlite3_open_v2("./Ayria/Client.db", &Ptr, SQLITE_OPEN_READONLY | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr);
            if (Result != SQLITE_OK) sqlite3_open_v2(":memory:", &Ptr, SQLITE_OPEN_READONLY | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr);
            Database = std::shared_ptr<sqlite3>(Ptr, [=](sqlite3 *Ptr) { sqlite3_close_v2(Ptr); });
        }

        return sqlite::database(Database);
    }

    // General information about the network.
    inline uint64_t getClientcount()
    {
        uint64_t Result{};

        try { Query() << "SELECT COUNT(*) FROM Clients;" >> Result; } catch (...) {}

        return Result;
    }
    inline JSON::Value_t getClientinfo(uint32_t AccountID)
    {
        JSON::Object_t Result{};

        try
        {
            Query()
                << "SELECT * FROM Clients WHERE AccountID = ?;" << AccountID
                >> [&](uint32_t, uint64_t Timestamp, uint32_t GameID, uint32_t ModID, const std::string &Username, const std::string &Encryptionkey)
                {
                    Result["Encryptionkey"] = Encryptionkey;
                    Result["AccountID"] = AccountID;
                    Result["Timestamp"] = Timestamp;
                    Result["Username"] = Username;
                    Result["GameID"] = GameID;
                    Result["ModID"] = ModID;
                };
        } catch (...) {}

        return Result;
    }
    inline JSON::Value_t getClients(std::optional<uint32_t> byGameID, std::optional<uint32_t> byModID)
    {
        std::unordered_set<uint32_t> AccountIDs{};
        JSON::Array_t Result{};

        try
        {
            if (byGameID && byModID)
            {
                Query()
                    << "SELECT AccountID FROM Clients WHERE (GameID = ? AND ModID = ?);"
                    << byGameID.value() << byModID.value()
                    >> [&](uint32_t AccountID)
                    {
                        AccountIDs.insert(AccountID);
                    };
            }
            else if (byGameID)
            {
                Query()
                    << "SELECT AccountID FROM Clients WHERE (GameID = ?);"
                    << byGameID.value()
                    >> [&](uint32_t AccountID)
                    {
                        AccountIDs.insert(AccountID);
                    };
            }
            else if (byModID)
            {
                Query()
                    << "SELECT AccountID FROM Clients WHERE ( ModID = ?);"
                    << byModID.value()
                    >> [&](uint32_t AccountID)
                    {
                        AccountIDs.insert(AccountID);
                    };
            }
            else
            {
                Query() << "SELECT AccountID FROM Clients;"
                        >> [&](uint32_t AccountID) { AccountIDs.insert(AccountID); };
            }
        } catch (...) {}

        Result.reserve(AccountIDs.size());
        for (const auto &ID : AccountIDs)
            Result.emplace_back(getClientinfo(ID));

        return Result;
    }

    // Client relationships.
    using isFriend = bool; using isBlocked = bool; using Relation = std::pair<isFriend, isBlocked>;
    inline std::pair<Relation, Relation> getRelation(uint32_t AccountA, uint32_t AccountB)
    {
        Relation A{}, B{};

        try
        {
            Query() << "SELECT isFriend, isBlocked FROM Relations WHERE (Source = ? AND Target = ?);"
                    << AccountA << AccountB >> std::tie(A.first, A.second);
            Query() << "SELECT isFriend, isBlocked FROM Relations WHERE (Source = ? AND Target = ?);"
                    << AccountB << AccountA >> std::tie(B.first, B.second);
        } catch (...) {}

        return { A, B };
    }
    inline bool areFriends(uint32_t AccountA, uint32_t AccountB)
    {
        const auto [A, B] = getRelation(AccountA, AccountB);
        return A.first && B.first;
    }
    inline bool areBlocked(uint32_t AccountA, uint32_t AccountB)
    {
        const auto [A, B] = getRelation(AccountA, AccountB);
        return A.second || B.second;
    }

    // Clients presence, key-value storage.
    using Presence = std::pair<std::string, std::string>; using Provider = std::string;
    inline std::unordered_map<Provider, std::vector<Presence>> dumpClientpresence(uint32_t AccountID)
    {
        std::unordered_map<Provider, std::vector<Presence>> Result{};

        try
        {
            Query()
                << "SELECT * FROM Presence WHERE AccountID = ?;"
                << AccountID
                >> [&](uint32_t, const std::string &Provider, const std::string &Key, const std::string &Value)
                {
                    Result[Provider].emplace_back(Key, Value);
                };
        } catch (...) {}

        return Result;
    }
    inline std::unordered_set<uint32_t> findClientpresence(const std::string &Provider, const std::string &Key)
    {
        std::unordered_set<uint32_t> Result{};

        try
        {
            Query()
                << "SELECT AccountID FROM Presence WHERE (Provider = ? AND Key = ?);"
                << Provider << Key
                >> [&](uint32_t AccountID) { Result.insert(AccountID); };
        } catch (...) {}

        return Result;
    }
    inline std::string getClientpresence(uint32_t AccountID, const std::string &Provider, const std::string &Key)
    {
        std::string Result{};

        try
        {
            Query()
                << "SELECT Value FROM Presence WHERE (AccountID = ? AND Provider = ? AND Key = ?);"
                << AccountID << Provider << Key >> Result;
        } catch (...) {}

        return Result;
    }


}
#endif

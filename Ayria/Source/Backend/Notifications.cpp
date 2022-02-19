/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2022-01-31
    License: MIT
*/

#include "Backend.hpp"

namespace Notifications
{
    static Hashmap<uint32_t, Hashset<CPPCallback_t>> NotificationCPP;
    static Hashmap<uint32_t, Hashset<CCallback_t>> NotificationC;

    // Duplicate subscriptions get ignored.
    void Subscribe(std::string_view Identifier, CCallback_t Handler)
    {
        if (Handler) [[likely]] NotificationC[Hash::WW32(Identifier)].insert(Handler);
    }
    void Subscribe(std::string_view Identifier, CPPCallback_t Handler)
    {
        if (Handler) [[likely]] NotificationCPP[Hash::WW32(Identifier)].insert(Handler);
    }
    void Unsubscribe(std::string_view Identifier, CCallback_t Handler)
    {
        NotificationC[Hash::WW32(Identifier)].erase(Handler);
    }
    void Unsubscribe(std::string_view Identifier, CPPCallback_t Handler)
    {
        NotificationCPP[Hash::WW32(Identifier)].erase(Handler);
    }

    // RowID is the row in Syncpackets that triggered the notification.
    void Publish(std::string_view Identifier, int64_t RowID, std::string_view Payload)
    {
        return Publish(Identifier, RowID, JSON::Parse(Payload).get<JSON::Object_t>());
    }
    void Publish(std::string_view Identifier, int64_t RowID, JSON::Value_t &&Payload)
    {
        const auto Hash = Hash::WW32(Identifier);

        // Always include a rowID, even if invalid.
        Payload["RowID"] = RowID;

        if (NotificationCPP.contains(Hash))
        {
            for (const auto &CB : NotificationCPP[Hash])
            {
                CB(Payload);
            }
        }

        if (NotificationC.contains(Hash))
        {
            const auto Plaintext = JSON::Dump(Payload);
            for (const auto &CB : NotificationC[Hash])
            {
                CB(Plaintext.data(), uint32_t(Plaintext.length()));
            }
        }
    }

    // Access from the plugins.
    namespace Export
    {
        extern "C" EXPORT_ATTR void __cdecl unsubscribeNotification(const char *Identifier, void(__cdecl *Callback)(const char *Message, unsigned int Length))
        {
            if (!Identifier || !Callback) [[unlikely]]
            {
                assert(false);
                return;
            }

            Unsubscribe(Identifier, Callback);
        }
        extern "C" EXPORT_ATTR void __cdecl subscribeNotification(const char *Identifier, void(__cdecl *Callback)(const char *Message, unsigned int Length))
        {
            if (!Identifier || !Callback) [[unlikely]]
            {
                assert(false);
                return;
            }

            Subscribe(Identifier, Callback);
        }
        extern "C" EXPORT_ATTR void __cdecl publishNotification(const char *Identifier, const char *Message, unsigned int Length)
        {
            if (!Identifier || !Message) [[unlikely]]
            {
                assert(false);
                return;
            }

            Publish(Identifier, -1, std::string_view{ Message, Length });
        }
    }
}

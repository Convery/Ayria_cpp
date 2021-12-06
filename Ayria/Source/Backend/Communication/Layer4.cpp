/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-10-05
    License: MIT
*/

#include <Global.hpp>

namespace Backend::Notifications
{
    static Hashmap<uint32_t, Hashset<Callback_t>> NotificationCB;

    void Subscribe(std::string_view Identifier, Callback_t Handler)
    {
        if (Handler) [[likely]] NotificationCB[Hash::WW32(Identifier)].insert(Handler);
    }
    void Unsubscribe(std::string_view Identifier, Callback_t Handler)
    {
        NotificationCB[Hash::WW32(Identifier)].erase(Handler);
    }

    void Publish(std::string_view Identifier, std::string_view Payload)
    {
        const auto Hash = Hash::WW32(Identifier);
        if (!Payload.ends_with('\0'))
        {
            static std::string Heap;
            Heap = Payload;
            Payload = Heap.c_str();
        }

        if (!NotificationCB.contains(Hash)) [[likely]] return;
        for (const auto &CB : NotificationCB[Hash]) CB(Payload.data(), (uint32_t)Payload.size());
    }
    void Publish(std::string_view Identifier, JSON::Object_t &&Payload)
    {
        const auto Hash = Hash::WW32(Identifier);
        const auto String = JSON::Dump(Payload);

        if (!NotificationCB.contains(Hash)) [[likely]] return;
        for (const auto &CB : NotificationCB[Hash]) CB(String.data(), (uint32_t)String.size());
    }
    void Publish(std::string_view Identifier, const JSON::Object_t &Payload)
    {
        const auto Hash = Hash::WW32(Identifier);
        const auto String = JSON::Dump(Payload);

        if (!NotificationCB.contains(Hash)) [[likely]] return;
        for (const auto &CB : NotificationCB[Hash]) CB(String.data(), (uint32_t)String.size());
    }

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

            Publish(Identifier, std::string_view{ Message, Length });
        }
    }
}

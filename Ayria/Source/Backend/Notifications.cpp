/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-08-12
    License: MIT
*/

#include <Global.hpp>

namespace Notifications
{
    static Hashmap<uint32_t, Hashset<Callback_t>> Notificationcallbacks{};

    void Register(std::string_view Identifier, Callback_t Handler)
    {
        const auto Hash = Hash::WW32(Identifier);
        Notificationcallbacks[Hash].insert(Handler);
    }
    void Publish(std::string_view Identifier, const char *JSONString)
    {
        const auto Hash = Hash::WW32(Identifier);
        const auto &Map = Notificationcallbacks[Hash];

        for (const auto &Handler : Map)
        {
            if (Handler) [[likely]] Handler(JSONString);
        }
    }

    // Access from the plugins..
    extern "C" EXPORT_ATTR void addNotificationlistener(const char *Identifier, void(__cdecl *Callback)(const char *JSONString))
    {
        if (!Identifier || !Callback) [[unlikely]] return;
        Register(Identifier, Callback);
    }
    extern "C" EXPORT_ATTR void publishNotification(const char *Identifier, const char *JSONString)
    {
        if (!Identifier || !JSONString) [[unlikely]] return;
        Publish(Identifier, JSONString);
    }
}

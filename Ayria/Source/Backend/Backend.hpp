/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-10-02
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>
#include <Global.hpp>

namespace Backend
{
    // Totally randomly selected constants..
    constexpr uint32_t Multicastaddress = Hash::FNV1_32("Ayria") << 8;  // 228.58.137.0
    constexpr uint16_t Matchmakeport = Hash::FNV1_32("Ayria") & 0xFFF0; // 14976
    constexpr uint16_t Fileshareport = Hash::FNV1_32("Ayria") & 0xFFF7; // 14977
    constexpr uint16_t Pluginsport = Hash::FNV1_32("Ayria") & 0xFFF8; // 14984
    constexpr uint16_t Generalport = Hash::FNV1_32("Ayria") & 0xFFFF; // 14985
    using Messagecallback_t = void(__cdecl *)(uint32_t NodeID, const char *JSONString);

    // Callbacks on group messages, groups are identified by their port, multicast is for all of Ayria.
    void Sendmessage(uint32_t Messagetype, std::string_view JSONString, uint16_t Port = Generalport);
    void Registermessagehandler(uint32_t MessageID, Messagecallback_t Callback);
    void Joinmessagegroup(uint16_t Port, uint32_t Address = Multicastaddress);

    // Add a recurring task to the worker thread.
    void Enqueuetask(uint32_t Period, void(__cdecl *Callback)());

    // Poll the internal socket(s).
    void Updatenetworking();

    // Initialize the system.
    void Initialize();

    // Helper for debug-builds.
    inline void setThreadname(std::string_view Name)
    {
        if constexpr (Build::isDebug)
        {
            #pragma pack(push, 8)
            using THREADNAME_INFO = struct { DWORD dwType; LPCSTR szName; DWORD dwThreadID; DWORD dwFlags; };
            #pragma pack(pop)

            __try
            {
                THREADNAME_INFO Info{ 0x1000, Name.data(), 0xFFFFFFFF };
                RaiseException(0x406D1388, 0, sizeof(Info) / sizeof(ULONG_PTR), (ULONG_PTR *)&Info);
            } __except (EXCEPTION_EXECUTE_HANDLER) {}
        }
    }

    // Add API handlers.
    inline std::string __cdecl Broadcastmessage(const char *JSONString)
    {
        do
        {
            if (!JSONString) break;

            const auto Object = ParseJSON(JSONString);
            if (!Object.contains("Messagetype")) break;
            if (!Object.contains("Message")) break;

            Sendmessage(Object["Messagetype"], Object["Message"], Pluginsport);

        } while (false);

        return "{}";
    }
    inline std::string __cdecl Joinmessagegroup(const char *JSONString)
    {
        do
        {
            if (!JSONString) break;

            const auto Object = ParseJSON(JSONString);
            if (!Object.contains("Address")) break;
            if (!Object.contains("Port")) break;

            Joinmessagegroup(Object["Port"], inet_addr(Object["Address"].get<std::string>().c_str()));
        } while (false);

        return "{}";
    }
    inline void API_Initialize()
    {
        API::Registerhandler_Network("Broadcastmessage", Broadcastmessage);
        API::Registerhandler_Network("Joinmessagegroup", Joinmessagegroup);
    }
}

/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-10-02
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>
#include <Global.hpp>

namespace Auxiliary
{
    // Totally randomly selected constants..
    constexpr uint32_t Multicastaddress = Hash::FNV1_32("Ayria") << 8;  // 228.58.137.0
    constexpr uint16_t Matchmakeport = Hash::FNV1_32("Ayria") & 0xFFF0; // 14976
    constexpr uint16_t Fileshareport = Hash::FNV1_32("Ayria") & 0xFFF7; // 14977
    constexpr uint16_t Pluginsport = Hash::FNV1_32("Ayria") & 0xFFF8; // 14984
    constexpr uint16_t Generalport = Hash::FNV1_32("Ayria") & 0xFFFF; // 14985
    using Messagecallback_t = void(__cdecl *)(const char *JSONString);

    // Callbacks on group messages, groups are identified by their port, multicast address is shared between all.
    void Sendmessage(uint32_t Messagetype, std::string_view JSONString, uint16_t Port = Generalport);
    void Registermessagehandler(uint32_t MessageID, Messagecallback_t Callback);
    void Joinmessagegroup(uint16_t Port = Generalport);

    // Poll the internal socket(s).
    void Updatenetworking();
}

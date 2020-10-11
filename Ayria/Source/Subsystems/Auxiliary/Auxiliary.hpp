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
    constexpr uint16_t Multicastport = Hash::FNV1_32("Ayria") & 0xFFFF; // 14985
    constexpr uint32_t Multicastaddress = Hash::FNV1_32("Ayria") << 8;  // 228.58.137.0

    // Callbacks on group messages.
    using Messagecallback_t = void(__cdecl *)(const char *JSONString);
    void Registermessagehandler(uint32_t MessageID, Messagecallback_t Callback);
    void Joinmessagegroup(uint32_t Address = Multicastaddress, uint16_t Port = Multicastport);
    void Sendmessage(uint32_t Messagetype, std::string_view JSONString);

    // Poll the internal socket(s).
    void Updatenetworking();
}

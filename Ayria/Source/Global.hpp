/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-04-14
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>

//namespace IPC
//{
//    constexpr size_t Instancecount = 12;
//
//    using Request_t = struct { std::string Subject, Content; };
//    using Callback_t = std::function<bool(const Request_t &Request, std::string &Response)>;
//
//    void addBroadcast(std::string_view Subject, std::string_view Content);
//    void addHandler(std::string_view Subject, Callback_t Callback);
//    void addBroadcast(Request_t &&Message);
//
//    extern "C" EXPORT_ATTR void Broadcast(const char *Subject, const char *Content);
//}
//
//namespace Console
//{
//
//    extern "C" EXPORT_ATTR void Consoleprint(const char *);
//    extern "C" EXPORT_ATTR void Consoleexec(const char *);
//}


// Initialize the subsystems.
void Ayriastartup();

namespace Networking
{
    using Request_t = struct { std::string Subject, Content; };
    using Callback_t = std::function<void(const Request_t &Request, std::string &Response)>;

    void addBroadcast(std::string_view Subject, std::string_view Content);
    void addHandler(std::string_view Subject, Callback_t Callback);
    void addBroadcast(Request_t &&Message);
    void onFrame();
}

namespace Console
{
    void onFrame();
}

namespace Loaders
{
    void Loadplugins();

    bool InstallTLSCallback();
    void RestoreTLSCallback();

    bool InstallEPCallback();
    void RestoreEPCallback();
}

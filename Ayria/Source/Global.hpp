/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-04-14
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>

/*
    Purpose:
    Matchmaking
    Social




    Build a table of LAN nodes, IPC nodes.
    Duplicate all messages to each, unified interface.



    export bool __cdecl onRequest(char *Request, char **Response); 















    Key-val lookup, matchmake, c




    Inter-module = DllExports
    Inter-network = UDP broadcast
    Inter-process = Piped

    IPC - Named pipe interface for remote tools.
    DLL -



    Have a request-wrapper to do JSON IPC


    export request();
    export


    void getProperty(char *Key,



    #define doAPICall(x, y) getproc(Ayria.dll, "onMessage")(x, y)
*/



namespace Networking
{
    using NodeID = uint64_t;
    std::vector<NodeID> getNodes();

}



namespace IPC
{
    constexpr size_t Instancecount = 12;

    using Request_t = struct { std::string Subject, Content; };
    using Callback_t = std::function<bool(const Request_t &Request, std::string &Response)>;

    void addBroadcast(std::string_view Subject, std::string_view Content);
    void addHandler(std::string_view Subject, Callback_t Callback);
    void addBroadcast(Request_t &&Message);

    extern "C" EXPORT_ATTR void Broadcast(const char *Subject, const char *Content);
}

namespace Console
{

    extern "C" EXPORT_ATTR void Consoleprint(const char *);
    extern "C" EXPORT_ATTR void Consoleexec(const char *);
}


namespace Networking
{
    using Request_t = struct { std::string Subject, Content; };
    using Callback_t = std::function<bool(const Request_t &Request, std::string &Response)>;

    void addBroadcast(std::string_view Subject, std::string_view Content);
    void addHandler(std::string_view Subject, Callback_t Callback);
    void addBroadcast(Request_t &&Message);

    extern "C" EXPORT_ATTR unsigned short __cdecl getNetworkport();
}

namespace Console
{

}


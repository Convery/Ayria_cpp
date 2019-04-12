/*
    Initial author: Convery (tcn@ayria.se)
    Started: 12-04-2019
    License: MIT
    Notes:
        This interface only defines callbacks for input.
        It's the implementer that needs to send to the
        socket / port (localhost) included in the call.

        Furthermore, plugins need to provide an export:
        IServer *Createserver(const char *Hostname);
*/

#pragma once
#include <cstdint>

struct IServer
{
    // Called whenever the client sends data to the associated hostname. We don't send statuses.
    void onDatagram(uint16_t Clientport, const void *Databuffer, const uint32_t Datasize) { };
    void onStream(const size_t Socket, const void *Databuffer, const uint32_t Datasize) { };
};

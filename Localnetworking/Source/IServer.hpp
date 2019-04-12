/*
    Initial author: Convery (tcn@ayria.se)
    Started: 12-04-2019
    License: MIT
    Notes:
        Plugins need to provide an export:
        IServer *Createserver(const char *Hostname);
*/

#pragma once
#include <cstdint>

struct IServer
{
    // If there's any issues, such as there being no data on the socket, return false.
    virtual bool onWrite(const void *Databuffer, const uint32_t Datasize) = 0;
    virtual bool onRead(void *Databuffer, uint32_t *Datasize) = 0;
    virtual void onConnect() = 0;
};

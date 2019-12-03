/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-04-12
    License: MIT
    Notes:
        Plugins need to provide an export:
        IServer *Createserver(const char *Hostname);
*/

#pragma once
#include <cstdint>

// Callbacks return false on error or if there's no data.
struct IServer
{
    struct Address_t { uint32_t IPv4; uint16_t Port; };
    struct Endpoints_t { Address_t Client, Server; };

    // Utility functionality.
    virtual void onConnect() {};
    virtual void onDisconnect() {};

    // Stream-based IO for protocols such as TCP.
    virtual bool onStreamread(void *Databuffer, uint32_t *Datasize) = 0;
    virtual bool onStreamwrite(const void *Databuffer, const uint32_t Datasize) = 0;

    // Packet-based IO for protocols such as UDP and ICMP.
    virtual bool onPacketread(void *Databuffer, uint32_t *Datasize) = 0;
    virtual bool onPacketwrite(const void *Databuffer, const uint32_t Datasize, const Endpoints_t *Endpoints) = 0;
};
struct IStreamserver : IServer
{
    // Nullsub packet-based IO.
    virtual bool onPacketread(void *, uint32_t *) { return false; }
    virtual bool onPacketwrite(const void *, const uint32_t, const Endpoints_t *) { return false; }
};
struct IDatagramserver : IServer
{
    // Nullsub stream-based IO.
    virtual bool onStreamread(void *, uint32_t *) { return false; }
    virtual bool onStreamwrite(const void *, const uint32_t) { return false; }
};

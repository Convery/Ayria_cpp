/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-08-26
    License: MIT
*/

#include "AABackend.hpp"
#include <openssl/curve25519.h>

namespace Communication
{
    // Keep the clients around for easier lookups.
    static Hashmap<uint32_t, std::array<uint8_t, 32>> Knownclients{};
    static Hashmap<Source_t, std::unordered_set<uint32_t>> Clientsources{};

    // Register handlers for the different packets message WW32(ID).
    static Hashmap<uint32_t /* Messagetype */, Hashset<Callback_t>> Generalhandlers{};
    static Hashmap<uint32_t /* Messagetype */, Hashset<Callback_t>> Targetedhandlers{};
    void registerHandler(std::string_view Identifier, Callback_t Handler, bool General)
    {
        return registerHandler(Hash::WW32(Identifier), Handler, General);
    }
    void registerHandler(uint32_t Identifier, Callback_t Handler, bool General)
    {
        assert(Handler);
        if (General) Generalhandlers[Identifier].insert(Handler);
        else Targetedhandlers[Identifier].insert(Handler);
    }

    // Save a packet for later processing or forward to the handlers, internal.
    void forwardPacket(uint32_t Identifier, uint32_t AccountID, std::string_view Payload, bool General)
    {
        if (General)
        {
            std::ranges::for_each(Generalhandlers[Identifier], [&](const auto &CB)
            { CB(AccountID, Payload.data(), static_cast<uint32_t>(Payload.size())); });
        }
        else
        {
            std::ranges::for_each(Targetedhandlers[Identifier], [&](const auto &CB)
            { CB(AccountID, Payload.data(), static_cast<uint32_t>(Payload.size())); });
        }
    }
    void savePacket(const Packet_t *Header, std::string_view Payload, Source_t Source)
    {
        const auto Timestamp = std::chrono::utc_clock::now().time_since_epoch().count();
        const auto To = Base58::Encode(Header->Payload.toPublic);
        const auto AccountID = Hash::WW32(Header->fromPublic);
        const auto From = Base58::Encode(Header->fromPublic);
        const auto Type = Header->Payload.Type;

        // Someone intentionally fucked with the packet.
        if (!Type || From.empty()) [[unlikely]] return;

        // Verify the packets signature (in-case someone forwarded it or it got corrupted).
        if (NULL == ED25519_verify(reinterpret_cast<const uint8_t*>(Payload.data()), Payload.size(),
                                   Header->Signature.data(), Header->fromPublic.data())) [[unlikely]] return;

        // Track the clients we've seen so far.
        if (!Knownclients.contains(AccountID)) [[unlikely]] Knownclients[AccountID] = Header->fromPublic;
        Clientsources[Source].insert(AccountID);

        try
        {
            if (Base85::isValid(Payload)) [[likely]]
            {
                Backend::Database()
                    << "INSERT INTO Messagelog (Timestamp, Type, From, To, Message) VALUES (?,?,?,?,?);"
                    << Timestamp << Type << From << To << std::string(Payload);
            }
            else
            {
                Backend::Database()
                    << "INSERT INTO Messagelog (Timestamp, Type, From, To, Message) VALUES (?,?,?,?,?);"
                    << Timestamp << Type << From << To << Base85::Encode(Payload);
            }
        } catch (...) {}
    }

    // Associate clients with a source for future lookups.
    std::unordered_set<uint32_t> enumerateSource(Source_t Source)
    {
        return Clientsources[Source];
    }

    // Publish a message to a clients source, or broadcast.
    bool Publish(std::string_view Identifier, std::string_view Payload, uint32_t TargetID)
    {
        return Publish(Hash::WW32(Identifier), Payload, TargetID);
    }
    bool Publish(uint32_t Identifier, std::string_view Payload, uint32_t TargetID)
    {
        constexpr auto Overhead = 32;
        if (TargetID && !Knownclients.contains(TargetID)) [[unlikely]] return false;

        // One can always dream..
        if (!Base85::isValid(Payload)) [[likely]]
        {
            static std::string Encoded{};
            Encoded = Base85::Encode(Payload);
            Payload = Encoded;
        }

        // Create the buffer with a bit of overhead in case the subsystem needs to modify it.
        const auto Buffersize = sizeof(Packet_t) + Payload.size() + Overhead;
        auto Buffer = std::make_unique<char []>(Buffersize);
        const auto Packet = (Packet_t *)Buffer.get();

        Packet->Payload.Type = Identifier;
        std::memcpy(&Buffer[sizeof(Packet_t)], Payload.data(), Payload.size());
        std::memcpy(Packet->fromPublic.data(), Global.SigningkeyPublic->data(), 32);
        if (TargetID) std::memcpy(Packet->Payload.toPublic.data(), Knownclients[TargetID].data(), 32);

        if (NULL == ED25519_sign(Packet->Signature.data(), (uint8_t *)&Packet->Payload,
                                 sizeof(Payload_t) + Payload.size(), Global.SigningkeyPrivate->data())) [[unlikely]] return false;

        // Forward to the correct subsystem.
        if (Clientsources[Source_t::ZMQ_CLIENT].contains(TargetID)) { assert(false); return true; }
        if (Clientsources[Source_t::ZMQ_SERVER].contains(TargetID)) { assert(false); return true; }

        // LAN subsystem as default / fallback.
        return Networking::LAN::Publish(std::move(Buffer), Buffersize, Overhead);
    }

    // Process all new messages twice a second.
    void __cdecl doProcess()
    {
        static std::chrono::utc_clock::duration Lasttick = { std::chrono::utc_clock::now().time_since_epoch() };
        const auto Timenow = std::chrono::utc_clock::now().time_since_epoch();
        const auto LongID = Global.getLongID();

        try
        {
            Backend::Database()
            << "SELECT (Type, From, To, Message) FROM Messagelog WHERE (Timestamp >= ? AND Timestamp < ?);"
            << Lasttick.count() << Timenow.count()
            >> [&](uint32_t Type, const std::string &From, const std::string &To, const std::string Message)
            {
                const auto Publickey = Base58::Decode(From);
                const auto AccountID = Hash::WW32(Publickey);
                const auto Decoded = Base85::Decode(Message);
                const auto General = To.empty() || To != LongID;
                forwardPacket(Type, AccountID, Decoded, General);
            };

            Lasttick = Timenow;
        } catch (...) {}
    }

    // Initialize the system.
    void Initialize()
    {
        try
        {
            Backend::Database() <<
                "CREATE TABLE IF NOT EXISTS Messagelog ("
                "Timestamp INTEGER NOT NULL, "
                "Type INTEGER NOT NULL, "
                "From TEXT NOT NULL, "
                "To TEXT NOT NULL, "
                "Message TEXT NOT NULL );";
        } catch (...) {}

        // Should be often enough.
        Backend::Enqueuetask(500, doProcess);
    }
}

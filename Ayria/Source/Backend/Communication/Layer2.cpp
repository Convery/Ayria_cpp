/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-10-05
    License: MIT
*/

#include <Global.hpp>

namespace Backend::Messageprocessing
{
    static Hashmap<uint32_t, Hashset<Callback_t>> Messagehandlers{};

    // Listen for packets of a certain type.
    void addMessagehandler(std::string_view Identifier, Callback_t Handler)
    {
        if (Handler) [[likely]] Messagehandlers[Hash::WW32(Identifier)].insert(Handler);
    }

    //
    static void __cdecl doProcess()
    {
        // Only processed if the queries succeed.
        Hashset<int64_t> Processed{}, Invalid{};

        // Poll for unprocessed packets.
        Backend::Database()
            << "SELECT rowid, Messagetype, Timestamp, Message, Sender FROM Messagestream WHERE (isProcessed = false) ORDER BY Timestamp LIMIT 10;"
            >> [&](int64_t rowid, uint32_t Messagetype, uint64_t Timestamp, const std::string &Message, const std::string &Sender)
            {
                // Decode generates less data, so re-use the buffer and ignore our const promise..
                const auto Payload = Base85::Decode(Message, const_cast<std::string &>(Message));
                Processed.insert(rowid);

                std::ranges::for_each(Messagehandlers[Messagetype], [&](const auto &CB)
                {
                    if (!CB(Timestamp, Sender.c_str(), Payload.data(), static_cast<uint32_t>(Payload.size())))
                    {
                        if (Global.Settings.pruneDB) Invalid.insert(rowid);
                    }
                });
            };

        // If we need pruning, get rid of the invalid entries.
        for (const auto &Row : Invalid)
        {
            Processed.erase(Row);

            Backend::Database()
                << "DELETE FROM Messagestream WHERE rowid = ?;"
                << Row;
        }

        // And mark the rest as processed.
        for (const auto &Row : Processed)
        {
            Backend::Database()
                << "UPDATE Messagestream SET isProcessed = true WHERE rowid = ?;"
                << Row;
        }
    }

    // Set up the system.
    void Initialize()
    {
        // TODO(tcn): Profile and decide on a proper rate..
        Backend::Enqueuetask(100, doProcess);
    }

    namespace Export
    {
        extern "C" void subscribeMessage(const char *Identifier, bool (__cdecl * Callback)(uint64_t Timestamp, const char *LongID, const char *Message, unsigned int Length))
        {
            if (Identifier && Callback) [[likely]]
                Messageprocessing::addMessagehandler(std::string_view{ Identifier }, Callback);
        }
    }
}

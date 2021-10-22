/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-10-05
    License: MIT
*/

#include <Global.hpp>

namespace Backend::Notifications
{
    static Hashmap<std::string, Hashset<Processor_t>> ProcessingCB;
    static Hashmap<uint32_t, Hashset<Callback_t>> NotificationCB;

    void Publish(std::string_view Identifier, const char *JSONString)
    {
        const auto Hash = Hash::WW32(Identifier);
        if (!NotificationCB.contains(Hash)) return;
        for (const auto &CB : NotificationCB[Hash]) CB(JSONString);
    }
    void Subscribe(std::string_view Identifier, Callback_t Handler)
    {
        if (Handler) [[likely]] NotificationCB[Hash::WW32(Identifier)].insert(Handler);
    }

    // Internal.
    void addProcessor(std::string_view Tablename, Processor_t Callback)
    {
        ProcessingCB[Tablename].insert(Callback);
    }

    // Get database changes and process.
    static void __cdecl doProcess()
    {
        for (const auto &[Table, Set] : ProcessingCB)
        {
            const auto Modified = Backend::getModified(Table);
            if (Modified.empty()) [[likely]] continue;

            for (const auto &CB : Set)
                for (const auto &Row : Modified)
                    CB(Row);
        }
    }

    // Set up the system.
    void Initialize()
    {
        Backend::Enqueuetask(200, doProcess);
    }
}

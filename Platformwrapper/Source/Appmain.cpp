/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-04-09
    License: MIT
*/

#include "Stdinclude.hpp"
#include "Common.hpp"

// Keep the global state together.
Globalstate_t Global{};

// Exported from the various platforms.
extern void Arclight_init();
extern void Tencent_init();
extern void Steam_init();

// Entrypoint when loaded as a shared library.
#if defined _WIN32
BOOLEAN __stdcall DllMain(HINSTANCE hDllHandle, DWORD nReason, LPVOID)
{
    if (nReason == DLL_PROCESS_ATTACH)
    {
        // Ensure that Ayrias default directories exist.
        std::filesystem::create_directories("./Ayria/Logs");
        std::filesystem::create_directories("./Ayria/Assets");
        std::filesystem::create_directories("./Ayria/Plugins");

        // Only keep a log for this session.
        Logging::Clearlog();

        // Opt out of further notifications.
        DisableThreadLibraryCalls(hDllHandle);

        // Although useless for most users, we create a console if we get a switch.
        #if defined(NDEBUG)
        if (std::strstr(GetCommandLineA(), "-devcon"))
        #endif
        {
            AllocConsole();
            freopen("CONOUT$", "w", stdout);
            freopen("CONOUT$", "w", stderr);
            SetConsoleTitleA("Platformwrapper Console");
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 8);
        }

        // Start tracking availability.
        Global.Startuptimestamp = time(NULL);

        // If there's a local bootstrap module, we'll load it and trigger TLS.
        constexpr auto Modulename = Build::is64bit ? "Ayria64.dll" : "Ayria32.dll";
        if(LoadLibraryA(("./"s + Modulename).c_str()) || LoadLibraryA(("./Ayria/"s + Modulename).c_str()))
            std::thread([]() { volatile bool NOP{}; (void)NOP; }).detach();
    }

    return TRUE;
}
#else
__attribute__((constructor)) void __stdcall DllMain()
{
    // Ensure that Ayrias default directories exist.
    std::filesystem::create_directories("./Ayria/Logs");
    std::filesystem::create_directories("./Ayria/Assets");
    std::filesystem::create_directories("./Ayria/Plugins");

    // Only keep a log for this session.
    Logging::Clearlog();
}
#endif

// Callback when loaded as a plugin.
extern "C" EXPORT_ATTR void __cdecl onStartup(bool)
{
    // Initialize the various platforms.
    Tencent_init();
    Steam_init();

    constexpr auto Modulename = Build::is64bit ? "./Ayria/Ayria64.dll" : "./Ayria/Ayria32.dll";
    if(const auto Handle = GetModuleHandleA(Modulename))
    {
        #define Import(x) Global.Ayria. x = (decltype(Global.Ayria. x))GetProcAddress(Handle, #x);
        Global.Ayria.Modulehandle = Handle;
        Import(addNetworkbroadcast);
        Import(addNetworklistener);
        Import(addConsolefunction);
        Import(addConsolemessage);
        Import(getLocalplayers);
        Import(getClientticket);
        Import(getClientname);
        Import(getClientID);
        #undef Import

        // Global event-listener.
        Global.Ayria.addNetworklistener("Platformwrapper", Communication::Messagehandler);
    }
}

// Broadcast events over Ayrias communications layer.
namespace Communication
{
    std::unordered_map<std::string, Eventcallback_t> *Messagecallbacks{};
    void addMessagecallback(std::string_view Eventtype, Eventcallback_t Callback)
    {
        if (!Messagecallbacks) Messagecallbacks = new std::remove_pointer_t<decltype(Messagecallbacks)>();
        Messagecallbacks->emplace(Eventtype, Callback);
    }
    void Broadcastmessage(std::string_view Eventtype, uint64_t TargetID, Bytebuffer &&Data)
    {
        if (const auto Callback = Global.Ayria.addNetworkbroadcast)
        {
            auto Object = nlohmann::json::object();
            Object["Sendername"] = Global.Username;
            Object["Eventdata"] = Data.asView();
            Object["SenderID"] = Global.UserID;
            Object["Eventtype"] = Eventtype;
            Object["TargetID"] = TargetID;

            Callback("Platformwrapper", Object.dump().c_str());
        }
    }
    void __cdecl Messagehandler(const char *Request)
    {
        try
        {
            const auto Object = nlohmann::json::parse(Request);
            const auto SenderID = Object.value("SenderID", uint64_t());
            const auto TargetID = Object.value("TargetID", uint64_t());
            const auto Sendername = Object.value("Sendername", std::string());
            const auto Eventtype = Object.value("Eventtype", std::string());
            const auto Eventdata = Object.value("Eventdata", Blob());

            if (Messagecallbacks->contains(Eventtype))
            {
                Messagecallbacks->at(Eventtype)(TargetID, SenderID, Sendername, Bytebuffer(Eventdata));
            }
        } catch (...) {}
    }
}

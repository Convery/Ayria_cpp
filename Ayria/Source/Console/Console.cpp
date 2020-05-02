/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-05-02
    License: MIT
*/

#include "../Global.hpp"

// TODO
namespace Console
{
    void addFunction(std::string_view Name, std::function<void(int, char **)> Callback)
    {}
    void addMessage(std::string_view Message, uint32_t Colour)
    {}

    void onStartup()
    {}
    void onFrame()
    {}
}


namespace API
{
    extern "C"
    {
        EXPORT_ATTR void __cdecl addConsolemessage(const char *String, unsigned int Colour)
        {
            Console::addMessage(String, Colour);
        }
        EXPORT_ATTR void __cdecl addConsolefunction(const char *Name, void *Callback)
        {
            Console::addFunction(Name, static_cast<void(__cdecl *)(int, char **)>(Callback));
        }
    }
}

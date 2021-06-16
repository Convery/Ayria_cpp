/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-06-06
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>
#include <Global.hpp>

namespace Console
{
    // UTF8 escaped ACII strings are passed to Argv for compatibility.
    using Functioncallback_t = void(__cdecl *)(int Argc, const char **Argv);
    using Logline_t = std::pair<std::wstring, Color_t>;

    // Threadsafe injection into and fetching from the global log.
    template <typename T> void addMessage(const std::basic_string<T> &Message, Color_t Color);
    template <typename T> void addMessage(std::basic_string_view<T> Message, Color_t Color);
    std::vector<Logline_t> getMessages(size_t Maxcount, std::wstring_view Filter);

    // Manage and execute the commandline, with optional logging.
    template <typename T> void addCommand(const std::basic_string<T> &Name, Functioncallback_t Callback);
    template <typename T> void addCommand(std::basic_string_view<T> Name, Functioncallback_t Callback);
    template <typename T> void execCommand(const std::basic_string<T> &Commandline, bool Log = true);
    template <typename T> void execCommand(std::basic_string_view<T> Commandline, bool Log = true);

    // Quake-style console for Windows.
    namespace Windows
    {
        // Show auto-creates a console if needed.
        void Createconsole(HINSTANCE hInstance);
        void Showconsole(bool Hide);
        void doFrame();
    }

    // Add common commands to the backend.
    void Initialize();
}


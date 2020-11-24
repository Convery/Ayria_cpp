/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-09-18
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>
#include <Global.hpp>

namespace Console
{
    // UTF8 escaped ASCII strings.
    using Callback_t = void(__cdecl *)(int Argc, const char **Argv);
    using Logline_t = std::pair<std::wstring, COLORREF>;

    // Threadsafe injection of strings into the global log.
    void addConsolemessage(const std::string &Message, Color_t Colour);

    // Fetch a copy of the internal strings.
    std::vector<Logline_t> getLoglines(size_t Count, std::wstring_view Filter);

    // Track the current filter.
    std::wstring_view getFilter();

    // Add a new command to the internal list.
    void addConsolecommand(std::string_view Name, Callback_t Callback);

    // Evaluate the string, optionally add to the history.
    void execCommandline(std::string_view Commandline, bool logCommand = true);

    // Quake-style console.
    namespace Windows
    {
        // Show auto-creates a console if needed.
        void Createconsole(HINSTANCE hInstance);
        void Showconsole(bool Hide);
        void doFrame();
    }

    // Ingame console.
    namespace Overlay
    {
        void Createconsole(Overlay_t<false> *Parent);
    }

    // Add common commands.
    void Initializebackend();
}


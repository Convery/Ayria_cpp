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
    using Callback_t = void(__cdecl *)(int Argc, wchar_t **Argv);
    using Logline_t = std::pair<std::wstring, COLORREF>;
    using Consoleinput_t = struct
    {
        std::wstring Lastcommand;
        std::wstring Inputline;
        size_t Eventcount;
        size_t Cursorpos;
    };

    // Threadsafe injection of strings into the global log.
    void addConsolemessage(const std::wstring &Message, COLORREF Colour);

    // Fetch a copy of the internal strings.
    std::vector<Logline_t> getLoglines(size_t Count, std::wstring_view Filter);

    // Add a new command to the internal list.
    void addConsolecommand(std::wstring_view Name, Callback_t Callback);

    // Evaluate the string, optionally add to the history.
    void execCommandline(std::wstring Commandline, bool logCommand = true);

    // TODO(tcn): DEPRECATE
    void onEvent(Eventflags_t Flags, std::variant<uint32_t, vec2_t, wchar_t> Data);
    void addConsoletab(const std::wstring &Name, const std::wstring &Filter);
    std::vector<std::pair<std::wstring, std::wstring>> getTabs();
    void setConsoleinput(std::wstring_view Input);
    const Consoleinput_t *getState();

    // Quake-style console.
    namespace Windows
    {
        // Show auto-creates a console if needed.
        void Createconsole(HINSTANCE hInstance);
        void Showconsole(bool Hide);
    }
}


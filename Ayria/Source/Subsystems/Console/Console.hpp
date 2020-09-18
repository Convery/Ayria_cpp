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
    using Logline_t = std::pair<std::string, COLORREF>;

    // Threadsafe injection of strings into the global log.
    void addConsolemessage(const std::string &Message, COLORREF Colour);

    // Fetch a copy of the internal strings.
    std::vector<Logline_t> getLoglines(size_t Count, std::string_view Filter);

    // Filter the input based on 'tabs'.
    void addConsoletab(const std::string &Name, const std::string &Filter);

    // Get all available filters.
    std::vector<std::pair<std::string, std::string>> getTabs();

    // Handle different forms of input.
    void onEvent(Eventflags_t Flags, std::variant<uint32_t, vec2_t, wchar_t> Data);

    // Update the consoles internal state.
    void setConsoleinput(std::wstring_view Input);

    // Add a new command to the internal list.
    void addConsolecommand(std::string_view Name, Callback_t Callback);
}


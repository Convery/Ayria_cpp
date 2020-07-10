/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-07-10
    License: MIT
*/

#include <Stdinclude.hpp>
#include "Console.hpp"

namespace Console
{
    using namespace Graphics;
    bool isLogvisible{};

    namespace Tabarea
    {
        // Changes since the last frame.
        std::atomic<int32_t> Eventcount;

        constexpr size_t Maxtabs = 10;
        int32_t Tabcount{1}, Selectedtab{ -1 };
        std::array<std::string, Maxtabs> Tabtitles;
        std::array<std::string, Maxtabs> Tabfilters;

        // Which user-selected filter is active?
        std::string_view getCurrentfilter()
        {
            // Default tab doesn't filter anything.
            if (Selectedtab == -1) return "";
            else return Tabfilters[Selectedtab];
        }

        // Filter the input based on 'tabs'.
        void addConsoletab(const std::string &Name, const std::string &Filter)
        {
            if (Tabcount >= Maxtabs) return;
            const auto String = std::string(Filter);

            // Sanity-check to avoid redundant filters.
            for (size_t i = 0; i < Maxtabs; ++i)
            {
                if (Tabfilters[i] == String)
                {
                    // Already added.
                    if (std::strstr(Tabtitles[i].c_str(), Name.c_str()))
                        return;

                    Tabtitles[i] += "| "s + String;
                    return;
                }
            }

            // Insert the requested filter.
            Tabtitles[Tabcount] = " "s + Name + " "s;
            Tabfilters[Tabcount] = String;
            Tabcount++;

            // State changed, redraw.
            Eventcount++;
        }
    }
    namespace Logarea
    {
        // Changes since the last frame.
        std::atomic<int32_t> Eventcount;

        constexpr size_t Logsize = 256;
        std::array<Logline_t, Logsize> Internalbuffer;
        nonstd::ring_span<Logline_t> Consolelog{ Internalbuffer.data(),
                                                 Internalbuffer.data() + Logsize,
                                                 Internalbuffer.data(), Logsize };

        // Fetch a copy of the internal strings.
        std::vector<Logline_t> getLoglines(size_t Count, std::string_view Filter)
        {
            std::vector<Logline_t> Result;
            Result.reserve(Count);

            std::for_each(Consolelog.rbegin(), Consolelog.rend(), [&](const auto &Item)
            {
                if (Count)
                {
                    if (std::strstr(Item.first.c_str(), Filter.data()))
                    {
                        Result.push_back(Item);
                        Count--;
                    }
                }
            });

            std::reverse(Result.begin(), Result.end());
            return Result;
        }

        // Threadsafe injection of strings into the global log.
        void addConsolemessage(const std::string &Message, COLORREF Colour)
        {
            static Spinlock Writelock;

            // Scan for keywords to detect a colour if not specified.
            if (Colour == 0)
            {
                Colour = [&]()
                {
                    // Common keywords.
                    if (std::strstr(Message.c_str(), "[E]") || std::strstr(Message.c_str(), "rror")) return 0xBE282A;
                    if (std::strstr(Message.c_str(), "[W]") || std::strstr(Message.c_str(), "arning")) return 0xBEC02A;

                    // Ayria default.
                    if (std::strstr(Message.c_str(), "[I]")) return 0x218FBD;
                    if (std::strstr(Message.c_str(), "[D]")) return 0x7F963E;
                    if (std::strstr(Message.c_str(), "[>]")) return 0x7F963E;

                    // Default.
                    return 0x315571;
                }();
            }

            // Safety per parse, rather than per line.
            const std::scoped_lock _(Writelock);

            // Split by newline.
            std::string_view Input(Message);
            while (!Input.empty())
            {
                if (const auto Pos = Input.find('\n'); Pos != std::string_view::npos)
                {
                    if (Pos != 0)
                    {
                        Consolelog.push_back({ Input.data(), Colour });
                    }
                    Input.remove_prefix(Pos + 1);
                    continue;
                }

                Consolelog.push_back({ Input.data(), Colour });
                break;
            }

            // State modified, redraw.
            Eventcount++;
        }
    }
    namespace Inputarea
    {
        // Changes since the last frame.
        std::atomic<int32_t> Eventcount;

        std::wstring Suggestionline{};
        std::wstring Lastcommand{};
        std::wstring Inputline{};
        uint32_t Cursorpos{};

        void setConsoleinput(std::wstring_view Input, std::wstring_view Suggestion)
        {
            Suggestionline = Suggestion;
            Inputline = Input;
            Eventcount++;
        }
        void execCommandline(std::wstring_view Commandline)
        {

        }

        struct Consoleinput_t : Element_t
        {
            COLORREF Backgroundcolour;
            COLORREF Suggestioncolour;
            COLORREF Bordercolour;
            COLORREF Inputcolour;

            Consoleinput_t()
            {
                Wantedevents.onCharinput = true;
                Wantedevents.doBackspace = true;
                Wantedevents.doCancel = true;
                Wantedevents.Keydown = true;
                Wantedevents.doPaste = true;
                Wantedevents.doEnter = true;
                Wantedevents.doTab = true;

                Position.x = 15;
                Size.y = 30;
            }

            void onEvent(Surface_t *Parent, Event_t Event) final
            {
                if (Event.Flags.onCharinput)
                {
                    Inputline.insert(Cursorpos, 1, Event.Data.Letter);
                    Eventcount++;
                    Cursorpos++;
                    return;
                }

                if (Event.Flags.doBackspace)
                {
                    Inputline.erase(Cursorpos + 1, 1);
                    Eventcount++;
                    Cursorpos--;
                    return;
                }

                if (Event.Flags.doEnter)
                {
                    execCommandline(Inputline);
                    Lastcommand = Inputline;
                    Suggestionline.clear();
                    Inputline.clear();
                    Cursorpos = 0;
                    Eventcount++;
                    return;
                }

                if (Event.Flags.doCancel)
                {
                    Suggestionline.clear();
                    Inputline.clear();
                    Cursorpos = 0;
                    Eventcount++;
                    return;
                }

                if (Event.Flags.doTab)
                {
                    Inputline = Suggestionline.substr(0, Suggestionline.find_first_of(L' '));
                    Cursorpos = Inputline.size();
                    Eventcount++;
                    return;
                }

                if (Event.Flags.doPaste)
                {
                    if (IsClipboardFormatAvailable(CF_UNICODETEXT) && OpenClipboard(NULL))
                    {
                        if (const auto Memory = GetClipboardData(CF_UNICODETEXT))
                        {
                            if (const auto Size = GlobalSize(Memory) - 1)
                            {
                                if (const auto String = (LPCWSTR)GlobalLock(Memory))
                                {
                                    Inputline.insert(Cursorpos, String);
                                    Cursorpos = Inputline.size();
                                    Eventcount++;
                                }
                            }

                            GlobalUnlock(Memory);
                        }

                        CloseClipboard();
                    }
                    return;
                }

                if (Event.Flags.Keydown)
                {
                    if (Event.Data.Keycode == VK_UP || Event.Data.Keycode == VK_DOWN)
                    {
                        std::swap(Lastcommand, Inputline);
                        Cursorpos = Inputline.size();
                        Eventcount++;
                        return;
                    }
                }
            }
            void onRender(Surface_t *Parent) final
            {
                Paint::Rectangle::Filled(Devicecontext, {}, Bordercolour, Size, 0, 1, Backgroundcolour);
                if (!Suggestionline.empty())
                {
                    Paint::Text::Opaque(Devicecontext, {}, Suggestioncolour,
                                        Fonts::getDefaultfont(), Suggestionline, Backgroundcolour);
                }
                Paint::Text::Transparant(Devicecontext, {}, Inputcolour, Fonts::getDefaultfont(), Inputline);
            }
        };
    }

    struct Console_t : Element_t
    {
        Inputarea::Consoleinput_t Inputelement;

        Console_t()
        {
            Wantedevents.Raw = 0xFFFFFFFF;
        }

        void onFrame(Surface_t *Parent, float) final
        {
            // OEM_5 seems to map to the key below ESC, '~' for some keyboards, '§' for others.
            const auto Toggledown = GetKeyState(VK_OEM_5) & (1U << 15);
            const auto Shiftdown = GetKeyState(VK_SHIFT) & (1U << 15);

            if (Toggledown && !Shiftdown)
            {
                // Preempt redrawing before showing.
                Inputarea::Eventcount++;
                Tabarea::Eventcount++;
                Logarea::Eventcount++;
                onRender(Parent);

                ShowWindowAsync(Parent->Windowhandle, SW_SHOW);
            }
        }
        void onEvent(Surface_t *Parent, Event_t Event) final
        {
            // Resize the window as needed.
            if (Event.Flags.onWindowchange)
            {
                // Root element, just copy the surface.
                Size = Parent->Size;

                // Move the log area around as needed.

                // Move the input area around as needed.
                Inputelement.Size.x = (Parent->Size.x - 30) / 2;
                if (!isLogvisible) Inputelement.Position.y = 30;
                else Inputelement.Position.y = 99 + 30; // TODO: Add log size.

                return;
            }

            // OEM_5 seems to map to the key below ESC, '~' for some keyboards, '§' for others.
            if (Event.Data.Keycode == VK_OEM_5)
            {
                if (Event.Flags.modShift)
                {
                    isLogvisible ^= true;
                    Logarea::Eventcount++;
                }
                else
                {
                    ShowWindowAsync(Parent->Windowhandle, SW_HIDE);
                }
            }

            // Forward to the elements.
            if (Event.Flags.Raw & Inputelement.Wantedevents.Raw) Inputelement.onEvent(Parent, Event);


        }
        void onRender(Surface_t *Parent) final
        {
            Element_t::onRender(Parent);

            Inputelement.onRender(Parent);
            BitBlt(Devicecontext, Inputelement.Position.x, Inputelement.Position.y, Inputelement.Size.x,
              Inputelement.Size.y, Inputelement.Devicecontext, Inputelement.Size.x, Inputelement.Size.y, SRCCOPY);


        }
    };

    // Create and assign the elements to the parent-surface.
    void Initialize(Surface_t *Parent)
    {
        Parent->Elements["Console"] = new Console_t();

    }
}

// Provide a C-API for external code.
namespace API
{
    extern "C" EXPORT_ATTR void __cdecl addConsolemessage(const char *String, unsigned int Colour)
    {
        assert(String);
        Console::Logarea::addConsolemessage(String, Colour);
    }
    extern "C" EXPORT_ATTR void __cdecl addConsoletab(const char *Name, const char *Filter)
    {
        assert(Name);
        assert(Filter);
        Console::Tabarea::addConsoletab(Name, Filter);
    }
}

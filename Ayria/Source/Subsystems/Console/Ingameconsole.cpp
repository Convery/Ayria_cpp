/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-09-18
    License: MIT
*/

#include <Stdinclude.hpp>
#include <Global.hpp>

namespace Console
{
    namespace Overlay
    {
        constexpr uint8_t Inputheight{ 25 }, Tabheight{ 25 };
        Overlay_t *Consoleoverlay;
        bool isExtended{};
        bool isVisible{};

        // void(__cdecl *onTick)(Element_t* This, float Deltatime);
        // void(__cdecl *onEvent)(Element_t* This, Eventflags_t Flags, std::variant<uint32_t, vec2_t, wchar_t> Data);


        namespace Outputarea
        {
            std::atomic<int32_t> Eventcount{};
            void __cdecl onEvent(Element_t* This, Eventflags_t Flags, std::variant<uint32_t, vec2_t, wchar_t> Data)
            {
                if (Flags.onWindowchange)
                {
                    assert(std::holds_alternative<vec2_t>(Data));
                    const auto Newsize = std::get<vec2_t>(Data);

                    const vec2_t Wantedsize{ Newsize.x, Newsize.y - Tabheight - Inputheight };
                    if (Wantedsize != This->Size)
                    {
                        Eventcount++;
                        This->Size = Wantedsize;
                        This->Surface = Consoleoverlay->Createsurface(This->Size);
                    }
                }
            }
            void __cdecl onTick(Element_t *This, float)
            {
                // We spend most ticks invisible.
                if (!isVisible) [[likely]] return;

                const auto Events = Eventcount.load();

                if (Events)
                {
                    // Default font-size should be 20.
                    const auto Linecount = This->Size.y / 20;
                    const auto Lines = Console::getLoglines(Linecount, L"");

                    // TODO(tcn): Paint.

                    This->Repainted = true;
                    Eventcount -= Events;
                }
            }
        }

        namespace Inputarea
        {
            std::atomic<int32_t> Eventcount{};
            std::wstring Lastcommand{};
            std::wstring Inputline{};
            bfloat16_t Elapsed{};
            size_t Cursorpos{};
            bool Caretstate{};

            void __cdecl onTick(Element_t *This, float Deltatime)
            {
                // We spend most ticks invisible.
                if (!isVisible) [[likely]] return;

                // Blink the caret.
                Elapsed += Deltatime;
                if (Elapsed > 1)
                {
                    Caretstate ^= true;
                    Eventcount++;
                    Elapsed = 0;
                }

                const auto Events = Eventcount.load();
                if (Events)
                {

                    // Split the string so we can have a caret.
                    std::wstring Output = Inputline.substr(0, Cursorpos);
                    Output.push_back(Caretstate ? L'|' : L' ');
                    Output.append(Inputline.substr(Cursorpos));

                    // TODO(tcn): Draw.

                    This->Repainted = true;
                    Eventcount -= Events;
                }
            }
            void __cdecl onEvent(Element_t *, Eventflags_t Flags, std::variant<uint32_t, vec2_t, wchar_t> Data)
            {
                if (Flags.onCharinput)
                {
                    assert(std::holds_alternative<wchar_t>(Data));
                    Inputline.insert(Cursorpos, 1, std::get<wchar_t>(Data));
                    Eventcount++;
                    Cursorpos++;
                    return;
                }

                if (Flags.doBackspace)
                {
                    if (Cursorpos)
                    {
                        Inputline.erase(Cursorpos + 1, 1);
                        Eventcount++;
                        Cursorpos--;
                    }
                    return;
                }

                if (Flags.doCancel)
                {
                    Inputline.clear();
                    Cursorpos = 0;
                    Eventcount++;
                    return;
                }

                if (Flags.doPaste)
                {
                    if (IsClipboardFormatAvailable(CF_UNICODETEXT) && OpenClipboard(NULL))
                    {
                        if (const auto Memory = GetClipboardData(CF_UNICODETEXT))
                        {
                            if ((GlobalSize(Memory) - 1) > 0)
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

                if (Flags.doEnter)
                {
                    Console::execCommandline(Inputline, false);
                    Inputline.clear();
                    Cursorpos = 0;
                    Eventcount++;
                    return;
                }

                // Key-press.
                if (Flags.Keydown)
                {
                    assert(std::holds_alternative<uint32_t>(Data));
                    const auto Keycode = std::get<uint32_t>(Data);

                    // History navigation.
                    if (Keycode == VK_UP || Keycode == VK_DOWN)
                    {
                        std::swap(Lastcommand, Inputline);
                        Cursorpos = Inputline.size();
                        Eventcount++;
                        return;
                    }

                    // Cursor navigation.
                    if (Keycode == VK_LEFT || Keycode == VK_RIGHT)
                    {
                        const auto Offset = Keycode == VK_LEFT ? -1 : 1;
                        Cursorpos = std::clamp(int32_t(Cursorpos + Offset), int32_t(), int32_t(Inputline.size()));
                        Eventcount++;
                        return;
                    }
                }
            }
        }

        namespace Frameprocessor
        {
            HWND Lastfocus{};
            uint32_t Previousclick{}, Previousmove{}, Lastmessage{};
            void __cdecl onTick(Element_t *, float)
            {
                // Only process if the current focus is on our process.
                DWORD ProcessID{};
                const auto Handle = GetForegroundWindow();
                const auto Overlayfocus = GetCurrentThreadId() == GetWindowThreadProcessId(Handle, &ProcessID);
                if (ProcessID != GetCurrentProcessId()) return;
                const auto Currenttick = GetTickCount();

                // OEM_5 seems to map to the key below ESC, '~' for some keyboards, '§' for others.
                if (GetKeyState(VK_OEM_5) & (1U << 15)) [[unlikely]]
                {
                    // Avoid duplicates by only updating every 200ms.
                    if (Currenttick - Previousclick > 200)
                    {
                        if (GetKeyState(VK_SHIFT) & 1U << 15)
                        {
                            const auto Newsize = Consoleoverlay->Size.y * (isExtended ? -0.5f : 1);
                            Consoleoverlay->setWindowsize({0, Newsize}, true);
                            isExtended ^= true;
                        }
                        else
                        {
                            isVisible ^= true;
                            if (isVisible) Lastfocus = Handle;
                        }

                        Consoleoverlay->setVisible(isVisible);
                        Previousclick = Currenttick;
                    }
                }

                // We spend most ticks invisible.
                if (!isVisible) [[likely]] return;

                // Check if the output area needs to be repainted.
                const auto Hash = Hash::FNV1_32(Console::getLoglines(1, L"")[0].first);
                if (Lastmessage != Hash) [[unlikely]]
                {
                    Outputarea::Eventcount++;
                    Lastmessage = Hash;
                }

                // Follow the app-window 10 times a second.
                if (Currenttick - Previousmove > 100)
                {
                    RECT Windowarea{};
                    GetWindowRect(Lastfocus, &Windowarea);

                    vec2_t Wantedsize{ Windowarea.right - Windowarea.left, Windowarea.bottom - Windowarea.top };
                    const vec2_t Position{ Windowarea.left, Windowarea.top + 40 };
                    Wantedsize.y *= isExtended ? 0.6f : 0.3f;

                    if (Consoleoverlay->Position != Position) Consoleoverlay->setWindowposition(Position);
                    if (Consoleoverlay->Size != Wantedsize) Consoleoverlay->setWindowsize(Wantedsize);
                    Previousmove = Currenttick;
                }
            }
        }

        // Show auto-creates a console if needed.
        void Createconsole(Overlay_t *Parent)
        {
            assert(Parent);
            Consoleoverlay = Parent;

            Element_t Frame{};
            Frame.onTick = Frameprocessor::onTick;
            Consoleoverlay->addElement(std::move(Frame));

            Element_t Output{};
            Output.onTick = Outputarea::onTick;
            Output.onEvent = Outputarea::onEvent;
            Output.Position = { 0, Tabheight, -1 };
            Output.Wantedevents.onWindowchange = true;
            Consoleoverlay->addElement(std::move(Output));

            // TODO(tcn): More.
        }
    }
}

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
        constexpr uint8_t Inputheight{ 30 };
        Overlay_t *Consoleoverlay;
        bool isExtended{};
        bool isVisible{};

        namespace Outputarea
        {
            std::atomic<int32_t> Eventcount{};
            void __cdecl onEvent(Element_t* This, Eventflags_t Flags, std::variant<uint32_t, vec2_t, wchar_t> Data)
            {
                if (Flags.onWindowchange)
                {
                    assert(std::holds_alternative<vec2_t>(Data));
                    const auto Newsize = std::get<vec2_t>(Data);

                    vec2_t Wantedsize{ Newsize.x, Newsize.y - Inputheight };
                    if (Wantedsize != This->Size)
                    {
                        Eventcount++;
                        This->Size = Wantedsize;
                        This->Surface = Consoleoverlay->Createsurface(This->Size, &This->Surface);
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
                    // Default font-size should be ~20.
                    const auto Linecount = (This->Size.y - 1) / 20;
                    const auto Lines = Console::getLoglines(Linecount, L"");

                    auto Renderer = Graphics(This->Surface);
                    Renderer.Clear(This->Size);

                    vec2_t Position{ 20, 5 }, Size{ This->Size.x - 40, This->Size.y };
                    Renderer.Quad(Position, Size).Solid(Color_t(39, 38, 35));
                    Renderer.Path(Position, { Position.x + Size.x, Position.y }).Outline(1, Color_t(0xBE, 0x90, 00));

                    Position.x += 10; Position.y += 5;

                    for (const auto &Item : Lines)
                    {
                        if (Item.first.empty()) continue;
                        Renderer.Text(Item.second).Opaque(Position, Item.first, Color_t(39, 38, 35));
                        Position.y += 19;
                    }

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
                    const std::wstring Output = Inputline.substr(0, Cursorpos) +
                        (Caretstate ? L'|' : L' ') + Inputline.substr(Cursorpos);
                    auto Renderer = Graphics(This->Surface);
                    Renderer.Clear(This->Size);

                    vec2_t Position{ 20, 0 }, Size{ This->Size.x - 40, This->Size.y };
                    Renderer.Quad(Position, Size).Filled(1, Color_t(0xBE, 0x90, 00), Color_t(39, 38, 35));

                    Position.x += 10; Position.y += 5;
                    Renderer.Text(Color_t(127, 150, 62)).Opaque(Position, Output, Color_t(39, 38, 35));

                    This->Repainted = true;
                    Eventcount -= Events;
                }
            }
            void __cdecl onEvent(Element_t *This, Eventflags_t Flags, std::variant<uint32_t, vec2_t, wchar_t> Data)
            {
                if (Flags.onWindowchange)
                {
                    assert(std::holds_alternative<vec2_t>(Data));
                    const auto Newsize = std::get<vec2_t>(Data);
                    const vec2_t Wantedsize{ Newsize.x, Inputheight };
                    const vec3_t Position{ 0, Newsize.y - Inputheight, 0 };

                    if (This->Position != Position) This->Position = { 0, Newsize.y - Inputheight, 0 };
                    if (This->Size != Wantedsize)
                    {
                        Eventcount++;
                        This->Size = Wantedsize;
                        This->Surface = Consoleoverlay->Createsurface(This->Size, &This->Surface);
                    }

                    if (isVisible) [[likely]] SetFocus(Consoleoverlay->Windowhandle);
                }

                if (Flags.onCharinput)
                {
                    assert(std::holds_alternative<wchar_t>(Data));
                    const auto Letter = std::get<wchar_t>(Data);

                    if (Letter == L'§' || Letter == L'½' || Letter == L'~')
                        return;

                    Inputline.insert(Cursorpos, 1, Letter);
                    Eventcount++;
                    Cursorpos++;
                    return;
                }

                if (Flags.doBackspace)
                {
                    if (Cursorpos)
                    {
                        Inputline.erase(Cursorpos - 1, 1);
                        Eventcount++;
                        Cursorpos--;
                    }
                    return;
                }

                if (Flags.doDelete)
                {
                    Inputline.erase(Cursorpos, 1);
                    Eventcount++;
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
                    Console::execCommandline(Inputline, true);
                    Lastcommand = Inputline;
                    Inputline.clear();
                    Cursorpos = 0;
                    Eventcount++;

                    // Notify the output that there's a new line.
                    Overlay::Outputarea::Eventcount++;
                    return;
                }

                if (Flags.doTab)
                {
                    // TODO(tcn): Try to auto-complete a command.
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
                const auto Currenttick = GetTickCount();
                const auto Handle = GetForegroundWindow();
                GetWindowThreadProcessId(Handle, &ProcessID);

                // OEM_5 seems to map to the key below ESC, '~' for some keyboards, '§' for others.
                if (ProcessID == GetCurrentProcessId() && GetAsyncKeyState(VK_OEM_5) & (1U << 15)) [[unlikely]]
                {
                    // Avoid duplicates by only updating every 200ms.
                    if (Currenttick - Previousclick > 200)
                    {
                        if (GetAsyncKeyState(VK_SHIFT) & 1U << 15)
                        {
                            const auto Newsize = Consoleoverlay->Size.y * (isExtended ? -0.5f : 1);
                            Consoleoverlay->setWindowsize({0, Newsize}, true);
                            isExtended ^= true;
                        }
                        else
                        {
                            isVisible ^= true;
                            if (isVisible)
                            {
                                Lastfocus = Handle;
                                Consoleoverlay->setVisible(false);
                            }
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
                    Inputarea::Eventcount++;
                    Lastmessage = Hash;
                }

                // Follow the app-window 10 times a second.
                if (Currenttick - Previousmove > 100)
                {
                    RECT Windowarea{};
                    GetWindowRect(Lastfocus, &Windowarea);

                    vec2_t Wantedsize{ Windowarea.right - Windowarea.left, Windowarea.bottom - Windowarea.top };
                    const vec2_t Position{ Windowarea.left, Windowarea.top + 45 };
                    Wantedsize.y *= isExtended ? 0.6f : 0.3f;

                    if (Consoleoverlay->Position != Position) Consoleoverlay->setWindowposition(Position);
                    if (Consoleoverlay->Size != Wantedsize) Consoleoverlay->setWindowsize(Wantedsize);
                    Previousmove = Currenttick;
                }
            }
            void __cdecl onEvent(Element_t *, Eventflags_t Flags, std::variant<uint32_t, vec2_t, wchar_t> Data)
            {
                // We spend most ticks invisible.
                if (!isVisible) [[likely]] return;

                // NOTE(tcn): Maybe a sync issue between drawing and hit-testing.
                // Hackery for Windows sometimes not activating the overlay.
                if (Flags.Mousedown)
                {
                    assert(std::holds_alternative<vec2_t>(Data));
                    const auto Position = std::get<vec2_t>(Data);
                    if (Position.x < 0 || Position.y < 0) return;

                    if (Position.x <= Consoleoverlay->Size.x && Position.y <= Consoleoverlay->Size.y)
                    {
                        SetForegroundWindow(Consoleoverlay->Windowhandle);
                    }
                }
            }
        }

        // Show auto-creates a console if needed.
        void Createconsole(Overlay_t *Parent)
        {
            assert(Parent);
            Consoleoverlay = Parent;

            Element_t Frame{};
            Frame.Wantedevents.Mousedown = true;
            Frame.onTick = Frameprocessor::onTick;
            Frame.onEvent = Frameprocessor::onEvent;
            Consoleoverlay->addElement(std::move(Frame));

            Element_t Output{};
            Output.onTick = Outputarea::onTick;
            Output.onEvent = Outputarea::onEvent;
            Output.Wantedevents.onWindowchange = true;
            Consoleoverlay->addElement(std::move(Output));

            Element_t Input{};
            Input.onTick = Inputarea::onTick;
            Input.onEvent = Inputarea::onEvent;
            Input.Wantedevents.Keydown = true;
            Input.Wantedevents.doEnter = true;
            Input.Wantedevents.doPaste = true;
            Input.Wantedevents.doDelete = true;
            Input.Wantedevents.doCancel = true;
            Input.Wantedevents.doBackspace = true;
            Input.Wantedevents.onCharinput = true;
            Input.Wantedevents.onWindowchange = true;
            Consoleoverlay->addElement(std::move(Input));

            // Add common commands.
            Initializebackend();

            // TODO(tcn): More.
        }
    }
}

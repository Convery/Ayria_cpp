/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2022-03-08
    License: MIT
*/


#include <Global.hpp>
using namespace Graphics;

constexpr auto Backgroundcolor = Color_t(39, 38, 35);
static bool isExtended = false;
static HWND Lastfocus{};

struct Outputarea_t : Elementinfo_t
{
    Outputarea_t()
    {
        ZIndex = 1;
        Eventmask.onWindowchange = true;

        onTick = [this](Window_t *Parent, uint32_t DeltaMS)
        {
            static uint32_t Lastmessage{};
            static int32_t Passedtime{};
            Passedtime += DeltaMS;

            if (Passedtime >= 100) [[unlikely]]
            {
                Passedtime -= 100;

                if (Console::LastmessageID != Lastmessage) [[unlikely]]
                {
                    Lastmessage = Console::LastmessageID;
                    Parent->Invalidatescreen(Position, Size);
                }
            }
        };

        onPaint = [this](Window_t *Parent, const struct Renderer_t &Renderer)
        {
            const auto Linecount = (Size.y - 4) / 16;
            const auto Lines = Console::getMessages(Linecount);

            Renderer.Rectangle(Position, Size)->Render({}, Backgroundcolor);
            Renderer.Line(Position, vec2i{ Size.x, Position.y })->Render(Color_t(0xBE, 0x90, 00), {});

            vec2u Offset = { 10, 4 };
            for (const auto &[String, Color] : Lines)
            {
                if (String.empty()) continue;

                // GDI seems to have an easier time if a background is provided.
                Renderer.Text(Offset, String)->Render(Color, Backgroundcolor);
                Offset.y += 16;
            }
        };

        onEvent = [this](Window_t *Parent, Eventflags_t Eventtype, const std::variant<uint32_t, vec2i> &Eventdata)
        {
            assert(std::holds_alternative<vec2i>(Eventdata));
            const auto [X, Y] = std::get<vec2i>(Eventdata);

            Size = { X, Y - 20 };
            Parent->Invalidatescreen(Position, Size);
        };
    }
} Outputarea{};

struct Inputarea_t : Elementinfo_t
{
    Ringbuffer_t<std::wstring, 5> Commandhistory{};
    size_t Cursorpos{};
    bool Caretstate{};

    Inputarea_t()
    {
        ZIndex = 2;
        Eventmask.onKey = true;
        Eventmask.onCharinput = true;
        Eventmask.onWindowchange = true;

        onTick = [this](Window_t *Parent, uint32_t DeltaMS)
        {
            static int32_t Passedtime{};
            Passedtime += DeltaMS;

            if (Passedtime >= 500) [[unlikely]]
            {
                Passedtime -= 500;
                Caretstate ^= 1;
                Parent->Invalidatescreen(Position, Size);
            }
        };

        onPaint = [this](Window_t *Parent, const struct Renderer_t &Renderer)
        {
            // Bounding box.
            Renderer.Rectangle(Position, Size)->Render({}, Backgroundcolor);
            Renderer.Line(Position, vec2i{ Size.x, Position.y })->Render(Color_t(0xBE, 0x90, 00), {});

            // The last element in our history should be the current input.
            const auto Inputline = Commandhistory.back();

            // Split the string so we can insert out caret.
            const std::wstring Output = Inputline.substr(0, Cursorpos) + (Caretstate ? L'|' : L' ') + Inputline.substr(Cursorpos);

            // A bit more cenetered.
            const vec2u Offset = { Position.x + 10, Position.y + 3 };
            Renderer.Text(Offset, Output)->Render(Color_t(127, 150, 62), Backgroundcolor);
        };

        onEvent = [this](Window_t *Parent, Eventflags_t Eventtype, const std::variant<uint32_t, vec2i> &Eventdata)
        {
            do
            {
                if (Eventtype.onWindowchange)
                {
                    assert(std::holds_alternative<vec2i>(Eventdata));
                    const auto [X, Y] = std::get<vec2i>(Eventdata);

                    Position = { 0, Y - 20 };
                    Size = { X, 20 };
                    break;
                }

                if (Eventtype.onCharinput)
                {
                    assert(std::holds_alternative<uint32_t>(Eventdata));
                    const auto Letter = (wchar_t)std::get<uint32_t>(Eventdata);

                    if (Letter == 0xA7 || Letter == 0xBD || Letter == L'~')
                        return;

                    Commandhistory.back().insert(Cursorpos, 1, Letter);
                    Cursorpos++;
                    break;
                }

                if (Eventtype.onKey && Eventtype.modDown)
                {
                    assert(std::holds_alternative<uint32_t>(Eventdata));
                    const auto Scancode = std::get<uint32_t>(Eventdata);

                    do
                    {
                        if (Scancode == VK_BACK && Cursorpos)
                        {
                            Commandhistory.back().erase(Cursorpos - 1, 1);
                            Cursorpos--;
                            break;
                        }

                        if (Scancode == VK_DELETE && Cursorpos < Commandhistory.back().size())
                        {
                            Commandhistory.back().erase(Cursorpos, 1);
                            break;
                        }

                        if (Scancode == VK_ESCAPE)
                        {
                            Commandhistory.back().clear();
                            Cursorpos = 0;
                            break;
                        }

                        if (Scancode == VK_RETURN)
                        {
                            auto String = Commandhistory.back();
                            String.erase(std::remove(String.begin(), String.end(), '\r'), String.end());
                            String.erase(std::remove(String.begin(), String.end(), '\n'), String.end());

                            Console::execCommand(String, true);

                            Commandhistory.Rotate(-1);
                            Commandhistory.back().clear();
                            Cursorpos = 0;
                            break;
                        }

                        if (Scancode == VK_PASTE)
                        {
                            if (IsClipboardFormatAvailable(CF_UNICODETEXT) && OpenClipboard(NULL))
                            {
                                if (const auto Memory = GetClipboardData(CF_UNICODETEXT))
                                {
                                    if ((GlobalSize(Memory) - 1) > 0)
                                    {
                                        if (const auto String = (LPCWSTR)GlobalLock(Memory))
                                        {
                                            Commandhistory.back().insert(Cursorpos, String);
                                            Cursorpos = Commandhistory.back().size();
                                        }
                                    }

                                    GlobalUnlock(Memory);
                                }

                                CloseClipboard();
                            }
                            break;
                        }

                        // History management.
                        if (Scancode == VK_UP)
                        {
                            Commandhistory.Rotate(1);
                            Cursorpos = Commandhistory.back().size();
                            break;
                        }
                        if (Scancode == VK_DOWN && !Commandhistory.back().empty())
                        {
                            Commandhistory.Rotate(-1);
                            Cursorpos = Commandhistory.back().size();
                            break;
                        }

                        // Cursor navigation.
                        if (Scancode == VK_LEFT || Scancode == VK_RIGHT)
                        {
                            const auto Offset = Scancode == VK_LEFT ? -1 : 1;
                            Cursorpos = std::clamp(int32_t(Cursorpos + Offset), int32_t(), int32_t(Commandhistory.back().size()));
                            break;
                        }

                        return;
                    } while (false);

                    break;
                }

            } while (false);

            Parent->Invalidatescreen(Position, Size);
        };
    }
} Inputarea{};

struct Boundingbox_t : Elementinfo_t
{
    Boundingbox_t()
    {
        onTick = [](Window_t *Parent, uint32_t)
        {
            static uint32_t Lastupdate{}, Lastmove{};

            const auto Resize = [&]()
            {
                if (!Parent->isVisible) return;

                RECT Area{};
                GetClientRect(Lastfocus, &Area);
                MapWindowPoints(Lastfocus, nullptr, reinterpret_cast<POINT *>(&Area), 2);

                const auto Parentwidth = std::abs(Area.right - Area.left);
                const auto Parentheight = std::abs(Area.bottom - Area.top);

                const vec2u Wantedsize{ std::min(Parentwidth - 20, 1440L), Parentheight * (isExtended ? 0.66f : 0.33f) + 0.5f };
                const vec2i Wantedposition{ Area.left + (Parentwidth / 2) - (Wantedsize.x / 2), Area.top + Parentheight * 0.01f };

                // Unlikely to be needed, but let's check.
                if (Parent->Windowposition != Wantedposition || Parent->Windowsize != Wantedsize) [[unlikely]]
                {
                    Parent->Resize(Wantedposition, Wantedsize);
                }
            };

            // Only process if the current focus is on our process.
            DWORD ProcessID{};
            const auto Currenttick = GetTickCount();
            const auto Handle = GetForegroundWindow();
            GetWindowThreadProcessId(Handle, &ProcessID);

            if (ProcessID == GetCurrentProcessId() && (GetAsyncKeyState(VK_OEM_3) & (1U << 15) || GetAsyncKeyState(VK_OEM_5) & (1U << 15))) [[unlikely]]
            {
                // Avoid duplicates by only updating every 200ms.
                if (Currenttick - Lastupdate > 200)
                {
                    if (GetAsyncKeyState(VK_SHIFT) & (1U << 15))
                    {
                        isExtended ^= 1;
                        Resize();
                    }
                    else
                    {
                        if (!Parent->isVisible)
                        {
                            Lastfocus = Handle;
                        }

                        Parent->Togglevisibility();
                        Resize();
                    }

                    Lastupdate = Currenttick;
                }
            }

            // If we are visible, we need to follow the main window.
            if (Parent->isVisible)
            {
                if (Parent->Windowhandle == GetForegroundWindow()) [[likely]]
                    return;

                // Avoid jitter by only updating every 33ms.
                if (Currenttick - Lastmove > 33)
                {
                    Lastmove = Currenttick;
                    Resize();
                }
            }

            return;
        };
    }
} Boundingbox{};

namespace Frontend
{
    // Overlay-console for a window.
    std::thread CreateOverlayconsole()
    {
        return std::thread([&]
        {
            // Name for debugging.
            setThreadname("Ayria_Consoleoverlay");

            static Overlay_t Overlay({}, {});
            Overlay.Insertelement(&Boundingbox);
            Overlay.Insertelement(&Outputarea);
            Overlay.Insertelement(&Inputarea);

            uint64_t Lastframe = GetTickCount64();
            while (true)
            {
                const auto Deltatime = GetTickCount64() - Lastframe;
                Overlay.onFrame(Deltatime);
                Lastframe += Deltatime;

                std::this_thread::sleep_for(std::chrono::milliseconds(std::clamp(32ULL - Deltatime, 0ULL, 16ULL)));
            }
        });
    }
}

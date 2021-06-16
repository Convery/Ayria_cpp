/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-06-11
    License: MIT
*/

#include <Stdinclude.hpp>
#include <Global.hpp>

// Process the consoles backlog and render.
namespace Outputarea
{
    static Elementinfo_t Elementinfo{};

    static void __cdecl onPaint(class Overlay_t *, const Graphics::Renderer_t &Renderer)
    {
        const auto Linecount = (Elementinfo.Size.y - 4) / 19;
        const auto Lines = Console::getMessages(Linecount, L"");

        Renderer.Rectangle({}, Elementinfo.Size)->Render({}, Color_t(39, 38, 35));
        Renderer.Line({}, vec2i{ Elementinfo.Size.x, 0 })->Render(Color_t(0xBE, 0x90, 00), {});

        vec2i Position = { 10, 4 };
        for (const auto &[String, Color] : Lines)
        {
            if (String.empty()) continue;

            // GDI seems to have an easier time if a background is provided.
            Renderer.Text(Position, String)->Render(Color, Color_t(39, 38, 35));
            Position.y += 19;
        }
    }
    static void __cdecl onEvent(class Overlay_t *Parent, Eventflags_t Eventtype, const std::variant<uint32_t, vec2i> &Eventdata)
    {
        if (Eventtype.onWindowchange) [[likely]]
        {
            assert(std::holds_alternative<vec2i>(Eventdata));
            auto Temp = std::get<vec2i>(Eventdata);
            Temp.y -= 35;

            if (Temp != Elementinfo.Size)
            {
                Elementinfo.Size = Temp;
                Parent->Invalidateelements();
            }
        }
    }
}

// Gather input from the user.
namespace Inputarea
{
    static Elementinfo_t Elementinfo{};
    static std::wstring Lastcommand{};
    static std::wstring Inputline{};
    static bfloat16_t Elapsed{};
    static size_t Cursorpos{};
    static bool Caretstate{};

    static void __cdecl onPaint(class Overlay_t *, const Graphics::Renderer_t &Renderer)
    {
        Renderer.Rectangle({ Elementinfo.Position.x - 2, Elementinfo.Position.y },
                           { Elementinfo.Size.x + 4, Elementinfo.Size.y }
                          )->Render(Color_t(0xBE, 0x90, 00), Color_t(39, 38, 35));

        // Split the string so that we can insert a caret.
        const std::wstring Output = Inputline.substr(0, Cursorpos) + (Caretstate ? L'|' : L' ') + Inputline.substr(Cursorpos);
        Renderer.Text({ Elementinfo.Position.x + 10, Elementinfo.Position.y + 7 }, Output)->Render(Color_t(127, 150, 62), Color_t(39, 38, 35));
    }
    static void __cdecl onEvent(class Overlay_t *Parent, Eventflags_t Eventtype, const std::variant<uint32_t, vec2i> &Eventdata)
    {
        if (Eventtype.onWindowchange) [[likely]]
        {
            assert(std::holds_alternative<vec2i>(Eventdata));

            const auto Windowsize = std::get<vec2i>(Eventdata);
            const vec2i Position = { 0, Windowsize.y - 35 };
            const vec2i Size = { Windowsize.x, 30};

            if (Position != Elementinfo.Position || Size != Elementinfo.Size)
            {
                Elementinfo.Size = Size;
                Elementinfo.Position = Position;
                Parent->Invalidateelements();
            }

            return;
        }

        if (Eventtype.onCharinput)
        {
            assert(std::holds_alternative<uint32_t>(Eventdata));
            const auto Letter = (wchar_t)std::get<uint32_t>(Eventdata);

            if (Letter == 0xA7 || Letter == 0xBD || Letter == L'~')
                return;

            Inputline.insert(Cursorpos, 1, Letter);
            Cursorpos++;

            Parent->Invalidatescreen(Elementinfo.Position, Elementinfo.Size);
            return;
        }

        if (Eventtype.onKey && Eventtype.modDown)
        {
            assert(std::holds_alternative<uint32_t>(Eventdata));
            const auto Scancode = std::get<uint32_t>(Eventdata);

            do
            {
                if (Scancode == VK_BACK && Cursorpos)
                {
                    Inputline.erase(Cursorpos - 1, 1);
                    Cursorpos--;
                    break;
                }

                if (Scancode == VK_DELETE && Cursorpos < Inputline.size())
                {
                    Inputline.erase(Cursorpos, 1);
                    break;
                }

                if (Scancode == VK_ESCAPE)
                {
                    Inputline.clear();
                    Cursorpos = 0;
                    break;
                }

                if (Scancode == VK_RETURN)
                {
                    if (Inputline.back() == '\n') Inputline.pop_back();
                    if (Inputline.back() == '\r') Inputline.pop_back();
                    Console::execCommand(Inputline, true);
                    Lastcommand = Inputline;
                    Inputline.clear();
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
                                    Inputline.insert(Cursorpos, String);
                                    Cursorpos = Inputline.size();
                                }
                            }

                            GlobalUnlock(Memory);
                        }

                        CloseClipboard();
                    }
                    break;
                }

                // History management.
                if (Scancode == VK_UP || Scancode == VK_DOWN)
                {
                    std::swap(Lastcommand, Inputline);
                    Cursorpos = Inputline.size();
                    break;
                }

                // Cursor navigation.
                if (Scancode == VK_LEFT || Scancode == VK_RIGHT)
                {
                    const auto Offset = Scancode == VK_LEFT ? -1 : 1;
                    Cursorpos = std::clamp(int32_t(Cursorpos + Offset), int32_t(), int32_t(Inputline.size()));
                    break;
                }

                return;
            } while (false);

            Parent->Invalidatescreen(Elementinfo.Position, Elementinfo.Size);
        }
    }

    static uint32_t Passedtime{};
    static void __cdecl onTick(class Overlay_t *Parent, uint32_t DeltatimeMS)
    {
        Passedtime += DeltatimeMS;
        if (Passedtime >= 1000)
        {
            Passedtime -= 1000;
            Caretstate ^= true;
            Parent->Invalidatescreen(Elementinfo.Position, Elementinfo.Size);
        }
    }
}

class Consoleoverlay_t : public Overlay_t
{
    HWND Lastfocus{};
    uint32_t Previousmove{};
    uint32_t Previousclick{};
    uint32_t Lastcaretblink{};
    uint32_t Lastmessagehash{};
    bool isVisible{}, isExtended{};

    protected:
    virtual void onTick(uint32_t DeltatimeMS)
    {
        DWORD ProcessID{};
        const auto Currenttick = GetTickCount();
        const auto Handle = GetForegroundWindow();
        GetWindowThreadProcessId(Handle, &ProcessID);

        // OEM_5 seems to map to the key below ESC, '~' for some keyboards.
        if (ProcessID == GetCurrentProcessId() && GetAsyncKeyState(VK_OEM_5) & (1U << 15)) [[unlikely]]
        {
            // Avoid duplicates by only updating every 200ms.
            if (Currenttick - Previousclick > 200)
            {
                if (GetAsyncKeyState(VK_SHIFT) & 1U << 15)
                {
                    isExtended ^= true;
                }
                else
                {
                    isVisible ^= true;

                    if (isVisible)
                    {
                        SetForegroundWindow(Windowhandle);
                        Lastfocus = Handle;
                    }
                    else
                    {
                        SetForegroundWindow(Lastfocus);
                        Lastfocus = 0;
                    }
                }

                ShowWindowAsync(Windowhandle, isVisible ? SW_SHOW : SW_HIDE);
                Previousclick = Currenttick;
            }
        }

        // We don't need to update the elements if invisible.
        if (!isVisible) [[likely]] return;

        // Check if there has been any new messages since the last frame.
        const auto Hash = Hash::WW32(Console::getMessages(1, L"")[0].first);
        if (Lastmessagehash != Hash) [[unlikely]]
        {
            // Notify that we need to redraw the console.
            Invalidatescreen(Outputarea::Elementinfo.Position, Outputarea::Elementinfo.Size);
            Lastmessagehash = Hash;
        }

        // Blink the caret for the input.
        if (Currenttick - Lastcaretblink > 1000)
        {
            Lastcaretblink = Currenttick;
            Inputarea::Caretstate ^= true;
            Invalidatescreen(Inputarea::Elementinfo.Position, Inputarea::Elementinfo.Size);
        }

        // Follow the main window and resize every 100ms.
        if (Currenttick - Previousmove > 100) [[unlikely]]
        {
            RECT Windowarea{};
            GetWindowRect(Lastfocus, &Windowarea);

            vec2i Wantedsize{ Windowarea.right - Windowarea.left - 40, Windowarea.bottom - Windowarea.top - 45 };
            const vec2i Wantedposition{ Windowarea.left + 20, Windowarea.top + 45 };
            Wantedsize.y *= isExtended ? 0.6f : 0.3f;

            // Unlikely to be needed, but let's check.
            if (Windowposition != Wantedposition || Windowsize != Wantedsize) [[unlikely]]
            {
                SetWindowPos(Windowhandle, NULL, Wantedposition.x, Wantedposition.y, Wantedsize.x, Wantedsize.y, SWP_ASYNCWINDOWPOS);
                SetFocus(Windowhandle);
            }

            Previousmove = Currenttick;
        }

        // Update all elements using the default function for easier debugging.
        Overlay_t::onTick(DeltatimeMS);
    }

    public:
    // Create all elements on startup with 0 size.
    Consoleoverlay_t() : Overlay_t({}, {})
    {
        Outputarea::Elementinfo.Eventmask.onWindowchange = true;
        Outputarea::Elementinfo.onEvent = Outputarea::onEvent;
        Outputarea::Elementinfo.onPaint = Outputarea::onPaint;
        Insertelement(&Outputarea::Elementinfo);

        Inputarea::Elementinfo.Eventmask.onWindowchange = true;
        Inputarea::Elementinfo.Eventmask.onCharinput = true;
        Inputarea::Elementinfo.Eventmask.onKey = true;
        Inputarea::Elementinfo.onPaint = Inputarea::onPaint;
        Inputarea::Elementinfo.onEvent = Inputarea::onEvent;
        Inputarea::Elementinfo.onTick = Inputarea::onTick;
        Insertelement(&Inputarea::Elementinfo);

        // Ensure that the basic commands are added.
        Console::Initialize();
    }
};

// Does what the name imples.
Overlay_t *Createconsoleoverlay()
{
    return new Consoleoverlay_t();
}

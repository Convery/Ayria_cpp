/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2022-03-08
    License: MIT
*/

#pragma once
#include <Global.hpp>

using namespace Graphics;

constexpr auto Backgroundcolor = Color_t(0xBE, 0x90, 00);
static bool isExtended = false;
static HWND Lastfocus{};

struct Outputarea_t : Elementinfo_t
{
    Outputarea_t()
    {
        ZIndex = 1;
        Eventmask.onWindowchange = true;

        onTick = [this](Overlay_t *Parent, uint32_t DeltaMS)
        {
            static uint32_t Lastmessage{};
            static int32_t Passedtime{};
            Passedtime += DeltaMS;

            if (Passedtime > 100) [[unlikely]]
            {
                Passedtime -= 100;

                if (Console::LastmessageID != Lastmessage) [[unlikely]]
                {
                    Lastmessage = Console::LastmessageID;
                    Parent->Invalidatescreen(Position, Size);
                }
            }
        };

        onPaint = [this](Overlay_t *Parent, const struct Renderer_t &Renderer)
        {
            const auto Linecount = (Size.y - 4) / 19;
            const auto Lines = Console::getMessages(Linecount);

            Renderer.Rectangle({0, 1}, Size)->Render({}, Color_t(39, 38, 35));
            Renderer.Line({}, vec2i{ Size.x, 0 })->Render(Color_t(0xBE, 0x90, 00), {});

            vec2u Position = { 10, 4 };
            for (const auto &[String, Color] : Lines)
            {
                if (String.empty()) continue;

                // GDI seems to have an easier time if a background is provided.
                Renderer.Text(Position, String)->Render(Color, Color_t(39, 38, 35));
                Position.y += 19;
            }
        };

        onEvent = [this](Overlay_t *Parent, Eventflags_t Eventtype, const std::variant<uint32_t, vec2i> &Eventdata)
        {
            assert(std::holds_alternative<vec2i>(Eventdata));
            const auto [X, Y] = std::get<vec2i>(Eventdata);

            Size = { X, Y - 20 };
            Parent->Invalidatescreen(Position, Size);
        };
    }
};

struct Inputarea_t : Elementinfo_t
{
    Inputarea_t()
    {
        ZIndex = 2;
        Eventmask.onKey = true;
        Eventmask.onCharinput = true;
        Eventmask.onWindowchange = true;
    }
};

struct Boundingbox_t : Elementinfo_t
{
    Boundingbox_t()
    {
        onTick = [](Overlay_t *Parent, uint32_t)
        {
            static uint32_t Lastupdate{}, Lastmove{};

            const auto Resize = [&]()
            {
                RECT Windowarea{};
                GetWindowRect(Lastfocus, &Windowarea);

                const vec2u Wantedsize{ Windowarea.right - Windowarea.left - 40, (Windowarea.bottom - Windowarea.top - 45) * (isExtended ? 0.6f : 0.3f) };
                const vec2i Wantedposition{ Windowarea.left + 20, Windowarea.top + 45 };

                // Unlikely to be needed, but let's check.
                if (Parent->Windowposition != Wantedposition || Parent->Windowsize != Wantedsize) [[unlikely]]
                {
                    Parent->Resize(Wantedposition, Wantedsize);
                }
            };

            // If we are visible, we need to follow the main window.
            if (Parent->isVisible)
            {
                if (Parent->Windowhandle == GetForegroundWindow()) [[likely]]
                    return;

                // Avoid jitter by only updating every 100ms.
                const auto Currenttick = GetTickCount();
                if (Currenttick - Lastmove > 100)
                {
                    Lastmove = Currenttick;
                    Resize();
                }
            }

            // OEM_5 seems to map to the key below ESC for some keyboards, OEM_3 for US.
            if (GetAsyncKeyState(VK_OEM_3) & (1U << 15) || GetAsyncKeyState(VK_OEM_5) & (1U << 15)) [[unlikely]]
            {
                // Avoid duplicates by only updating every 200ms.
                const auto Currenttick = GetTickCount();
                if (Currenttick - Lastupdate > 200)
                {
                    if (GetAsyncKeyState(VK_SHIFT) & (1U << 15))
                    {
                        isExtended ^= 1;
                        Resize();
                    }
                    else
                    {
                        if (!Parent->isVisible) Lastfocus = GetForegroundWindow();
                        Parent->Togglevisibility();
                    }
                }

                Lastupdate = Currenttick;
            }
        };
    }
};

namespace Frontend
{


    // Overlay-console for a window.
    std::thread CreateOverlayconsole()
    {
        static Boundingbox_t Box{};
        static Outputarea_t Output{};

        static Overlay_t Overlay({}, {});
        Overlay.Insertelement(&Box);
        Overlay.Insertelement(&Output);

        return std::thread([&]
        {
            // Disable DPI scaling on Windows 10.
            if (const auto Callback = GetProcAddress(GetModuleHandleA("User32.dll"), "SetThreadDpiAwarenessContext"))
                reinterpret_cast<size_t(__stdcall *)(size_t)>(Callback)(static_cast<size_t>(-2));

            // Name for debugging.
            setThreadname("Ayria_Consoleoverlay");

            uint64_t Lastframe = GetTickCount64();
            while (true)
            {
                const auto Deltatime = GetTickCount64() - Lastframe;
                Overlay.onFrame(Deltatime);
                Lastframe += Deltatime;

                std::this_thread::sleep_for(std::chrono::milliseconds(std::clamp(16ULL - Deltatime, 0ULL, 16ULL)));
            }
        });
    }
}

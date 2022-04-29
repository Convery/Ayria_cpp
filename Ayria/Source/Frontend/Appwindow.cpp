/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2022-04-17
    License: MIT
*/

#include <Global.hpp>
using namespace Graphics;

constexpr auto Buttonwidth = 34;
constexpr auto Buttonspacing = 2;
constexpr auto Toolbarheight = 36;

// Background and used for moving the window around.
struct Toolbararea_t : Elementinfo_t
{
    static constexpr auto Backgroundcolor = Color_t(0x15, 0x15, 0x15);
    std::jthread Moverthread;

    Toolbararea_t()
    {
        ZIndex = 1;
        Eventmask.onClick = true;
        Eventmask.onWindowchange = true;

        onPaint = [this](Window_t *Parent, const struct Renderer_t &Renderer)
        {
            Renderer.Rectangle(Position, Size)->Render({}, Backgroundcolor);
        };

        onEvent = [this](Window_t *Parent, Eventflags_t Eventtype, const std::variant<uint32_t, vec2i> &Eventdata)
        {
            if (Eventtype.onWindowchange)
            {
                assert(std::holds_alternative<vec2i>(Eventdata));
                const auto [X, Y] = std::get<vec2i>(Eventdata);

                Size = { X, Toolbarheight };
                Parent->Invalidatescreen(Position, Size);
            }

            // Eventtype.onClick
            else if (isFocused.test())
            {
                assert(std::holds_alternative<uint32_t>(Eventdata));
                const auto Button = std::get<uint32_t>(Eventdata);

                // Windows should translate this for left-handed users.
                if (Button == VK_LBUTTON)
                {
                    if (Eventtype.modDown)
                    {
                        // Windows does not send updates about the mouse moving fast enough.
                        Moverthread = std::jthread([=](std::stop_token Token)
                        {
                            POINT Lastmouse{};
                            GetCursorPos(&Lastmouse);

                            // In-case this was actually a click, hold off for a frame.
                            std::this_thread::sleep_for(std::chrono::milliseconds(30));

                            while (!Token.stop_requested())
                            {
                                POINT Currentmouse{};
                                GetCursorPos(&Currentmouse);

                                if (Lastmouse.x + Lastmouse.y == 0) Lastmouse = Currentmouse;
                                Parent->Reposition({}, vec2i{ Currentmouse.x - Lastmouse.x, Currentmouse.y - Lastmouse.y });

                                Lastmouse = Currentmouse;
                                std::this_thread::sleep_for(std::chrono::milliseconds(30));
                            }
                        });
                    }
                    else
                    {
                        Moverthread.request_stop();
                    }
                }
            }
        };
    }
} Toolbararea{};



struct Closebutton_t : Elementinfo_t
{
    static constexpr auto Foregroundcolor = Color_t(0xFF, 0, 0);
    bool Primed{};

    Closebutton_t()
    {
        ZIndex = 2;
        Eventmask.onClick = true;
        Eventmask.onWindowchange = true;

        onPaint = [this](Window_t *Parent, const struct Renderer_t &Renderer)
        {
            if (Primed || isFocused.test())
            {
                Renderer.Textcentered({ Position.x, Position.y - 2, Position.x + Size.x, Position.y + Size.y - 2 }, L"X")
                    ->Render(Color_t(0, 0, 0), Foregroundcolor);
            }
            else
            {
                Renderer.Rectangle({ Position.x, Position.y - 2 }, Size)->Render(Foregroundcolor, {});
                Renderer.Textcentered({ Position.x, Position.y - 2, Position.x + Size.x, Position.y + Size.y - 2 }, L"X")
                    ->Render(Color_t(0, 0xFF, 0), {});
            }            
        };

        onEvent = [this](Window_t *Parent, Eventflags_t Eventtype, const std::variant<uint32_t, vec2i> &Eventdata)
        {
            if (Eventtype.onWindowchange)
            {
                assert(std::holds_alternative<vec2i>(Eventdata));
                const auto [X, Y] = std::get<vec2i>(Eventdata);

                Size = { Buttonwidth, Toolbarheight * 0.5f};
                Position = { Toolbararea.Size.x - 1 * (Size.x + Buttonspacing), 0 };
                Parent->Invalidatescreen(Position, Size);
            }

            // Eventtype.onClick
            else if (isFocused.test())
            {
                assert(std::holds_alternative<uint32_t>(Eventdata));
                const auto Button = std::get<uint32_t>(Eventdata);

                // Windows should translate this for left-handed users.
                if (Button == VK_LBUTTON)
                {
                    if (Eventtype.modDown) Primed = true;
                    else
                    {
                        if (Primed) Core::Terminate();
                        else Primed = false;
                    }
                }
            }
            else
            {
                Primed = false;
            }
        };

        onTick = [this](Window_t *Parent, uint32_t)
        {
            static bool Old = isFocused.test();
            if (Old != isFocused.test()) [[unlikely]]
            {
                Old = isFocused.test();
                Parent->Invalidatescreen(Position, Size);
            }
        };
    }

} Closebutton{};

struct Maxbutton_t : Elementinfo_t
{
    static constexpr auto Foregroundcolor = Color_t(0xFF, 0, 0);
    bool Primed{};

    Maxbutton_t()
    {
        ZIndex = 2;
        Eventmask.onClick = true;
        Eventmask.onWindowchange = true;

        onPaint = [this](Window_t* Parent, const struct Renderer_t& Renderer)
        {
            if (Primed || isFocused.test())
            {
                Renderer.Textcentered({ Position.x, Position.y - 2, Position.x + Size.x, Position.y + Size.y - 2 }, L"□")
                    ->Render(Color_t(0, 0, 0), Foregroundcolor);
            }
            else
            {
                Renderer.Rectangle({ Position.x, Position.y - 2 }, Size)->Render(Foregroundcolor, {});
                Renderer.Textcentered({ Position.x, Position.y - 2, Position.x + Size.x, Position.y + Size.y - 2 }, L"□")
                    ->Render(Color_t(0, 0xFF, 0), {});
            }
        };

        onEvent = [this](Window_t* Parent, Eventflags_t Eventtype, const std::variant<uint32_t, vec2i>& Eventdata)
        {
            if (Eventtype.onWindowchange)
            {
                assert(std::holds_alternative<vec2i>(Eventdata));
                const auto [X, Y] = std::get<vec2i>(Eventdata);

                Size = { Buttonwidth, Toolbarheight * 0.5f };
                Position = { Toolbararea.Size.x - 2 * (Size.x + Buttonspacing), 0 };
                Parent->Invalidatescreen(Position, Size);
            }

            // Eventtype.onClick
            else if (isFocused.test())
            {
                assert(std::holds_alternative<uint32_t>(Eventdata));
                const auto Button = std::get<uint32_t>(Eventdata);

                // Windows should translate this for left-handed users.
                if (Button == VK_LBUTTON)
                {
                    if (Eventtype.modDown) Primed = true;
                    else
                    {
                        if (Primed)
                        {
                            const auto hDC = GetDC(GetDesktopWindow());
                            const auto scrWidth = GetDeviceCaps(hDC, HORZRES);
                            const auto scrHeight = GetDeviceCaps(hDC, VERTRES);
                            ReleaseDC(GetDesktopWindow(), hDC);

                            if (Parent->Windowsize.x != scrWidth || Parent->Windowsize.y != scrHeight)
                                ShowWindowAsync(Parent->Windowhandle, SW_MAXIMIZE);
                            else
                                ShowWindowAsync(Parent->Windowhandle, SW_RESTORE);
                        }
                        Primed = false;
                    }
                }
            }
            else
            {
                Primed = false;
            }
        };

        onTick = [this](Window_t* Parent, uint32_t)
        {
            static bool Old = isFocused.test();
            if (Old != isFocused.test()) [[unlikely]]
            {
                Old = isFocused.test();
                Parent->Invalidatescreen(Position, Size);
            }
        };
    }

} Maxbutton{};

struct Minbutton_t : Elementinfo_t
{
    static constexpr auto Foregroundcolor = Color_t(0xFF, 0, 0);
    bool Primed{};

    Minbutton_t()
    {
        ZIndex = 2;
        Eventmask.onClick = true;
        Eventmask.onWindowchange = true;

        onPaint = [this](Window_t* Parent, const struct Renderer_t& Renderer)
        {
            if (Primed || isFocused.test())
            {
                Renderer.Textcentered({ Position.x, Position.y - 2, Position.x + Size.x, Position.y + Size.y - 2 }, L"_")
                    ->Render(Color_t(0, 0, 0), Foregroundcolor);
            }
            else
            {
                Renderer.Rectangle({ Position.x, Position.y - 2 }, Size)->Render(Foregroundcolor, {});
                Renderer.Textcentered({ Position.x, Position.y - 2, Position.x + Size.x, Position.y + Size.y - 2 }, L"_")
                    ->Render(Color_t(0, 0xFF, 0), {});
            }
        };

        onEvent = [this](Window_t* Parent, Eventflags_t Eventtype, const std::variant<uint32_t, vec2i>& Eventdata)
        {
            if (Eventtype.onWindowchange)
            {
                assert(std::holds_alternative<vec2i>(Eventdata));
                const auto [X, Y] = std::get<vec2i>(Eventdata);

                Size = { Buttonwidth, Toolbarheight * 0.5f };
                Position = { Toolbararea.Size.x - 3 * (Size.x + Buttonspacing), 0 };
                Parent->Invalidatescreen(Position, Size);
            }

            // Eventtype.onClick
            else if (isFocused.test())
            {
                assert(std::holds_alternative<uint32_t>(Eventdata));
                const auto Button = std::get<uint32_t>(Eventdata);

                // Windows should translate this for left-handed users.
                if (Button == VK_LBUTTON)
                {
                    if (Eventtype.modDown) Primed = true;
                    else
                    {
                        if (Primed) ShowWindowAsync(Parent->Windowhandle, SW_MINIMIZE);
                        Primed = false;
                    }
                }
            }
            else
            {
                Primed = false;
            }
        };

        onTick = [this](Window_t* Parent, uint32_t)
        {
            static bool Old = isFocused.test();
            if (Old != isFocused.test()) [[unlikely]]
            {
                Old = isFocused.test();
                Parent->Invalidatescreen(Position, Size);
            }
        };
    }

} Minbutton{};

namespace Frontend
{
    // Graphical UI for the client.
    std::thread CreateAppwindow()
    {
        return std::thread([&]
        {
            // Name for debugging.
            setThreadname("Ayria_Appwindow");

            const auto hDC = GetDC(GetDesktopWindow());
            const auto scrWidth = GetDeviceCaps(hDC, HORZRES);
            const auto scrHeight = GetDeviceCaps(hDC, VERTRES);
            ReleaseDC(GetDesktopWindow(), hDC);

            const vec2i Windowsize = { 1280, 720 };
            static Graphics::Window_t App(Windowsize, { (scrWidth - Windowsize.x) / 2, (scrHeight - Windowsize.y) / 2 });

            App.Insertelement(&Toolbararea);
            App.Insertelement(&Closebutton);
            App.Insertelement(&Maxbutton);
            App.Insertelement(&Minbutton);

            uint64_t Lastframe = GetTickCount64();
            while (true)
            {
                const auto Deltatime = GetTickCount64() - Lastframe;
                App.onFrame(Deltatime);
                Lastframe += Deltatime;

                std::this_thread::sleep_for(std::chrono::milliseconds(std::clamp(32ULL - Deltatime, 0ULL, 16ULL)));
            }
        });
    }
}
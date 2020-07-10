/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-07-04
    License: MIT
*/

#include <Stdinclude.hpp>
#include "Graphics.hpp"

namespace Graphics
{
    std::unordered_map<std::string, Surface_t> Surfaces;
    constexpr auto Clearcolor = toColour(0xFF, 0xFF, 0xFF, 0xFF);
    const HBRUSH Clearbrush = CreateSolidBrush(Clearcolor);

    // Create a new window and update them.
    Surface_t *Createsurface(std::string_view Identifier, Surface_t *Original)
    {
        // Register the window.
        WNDCLASSEXA Windowclass{};
        Windowclass.hbrBackground = Clearbrush;
        Windowclass.cbSize = sizeof(WNDCLASSEXA);
        Windowclass.lpfnWndProc = DefWindowProcW;
        Windowclass.lpszClassName = Identifier.data();
        Windowclass.hInstance = GetModuleHandleA(NULL);
        Windowclass.style = CS_SAVEBITS | CS_BYTEALIGNWINDOW | CS_BYTEALIGNCLIENT | CS_OWNDC;
        if (NULL == RegisterClassExA(&Windowclass)) return nullptr;

        // Topmost, optionally transparent, no icon on the taskbar.
        const auto Windowhandle = CreateWindowExA(WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
                                                  Windowclass.lpszClassName, NULL, WS_POPUP, NULL, NULL,
                                                  NULL, NULL, NULL, NULL, Windowclass.hInstance, NULL);
        if (!Windowhandle) return nullptr;

        // Use a pixel-value to mean transparent rather than Alpha, because using Alpha is slow.
        SetLayeredWindowAttributes(Windowhandle, Clearcolor, 0, LWA_COLORKEY);

        // Append to a pre-allocated surface.
        if (Original)
        {
            Original->Windowhandle = Windowhandle;
            Original->Devicecontext = CreateCompatibleDC(GetDC(Windowhandle));
            return &Surfaces.emplace(Identifier.data(), std::move(Original)).first->second;
        }
        else
        {
            auto Entry = &Surfaces[Identifier.data()];
            Entry->Windowhandle = Windowhandle;
            Entry->Devicecontext = CreateCompatibleDC(GetDC(Windowhandle));
            return Entry;
        }
    }

    inline void Processinput()
    {
        POINT Globalmouse{};
        GetCursorPos(&Globalmouse);

        for (auto &[Name, Surface] : Surfaces)
        {
            const bool Ctrl = static_cast<const bool>(GetKeyState(VK_CONTROL) & (1 << 15));
            const bool Shift = static_cast<const bool>(GetKeyState(VK_SHIFT) & (1 << 15));

            MSG Message;
            while (PeekMessageA(&Message, Surface.Windowhandle, NULL, NULL, PM_REMOVE))
            {
                Event_t Event{};
                TranslateMessage(&Message);
                Event.Flags.modCtrl = Ctrl;
                Event.Flags.modShift = Shift;

                switch (Message.message)
                {
                    // Window changed.
                    case WM_SIZE:
                    {
                        const uint16_t Width = LOWORD(Message.lParam);
                        const uint16_t Height = HIWORD(Message.lParam);

                        if (Width != Surface.Size.x || Height != Surface.Size.y)
                        {
                            const auto Bitmap = CreateCompatibleBitmap(GetDC(Message.hwnd), Width, Height);
                            const auto Old = SelectObject(Surface.Devicecontext, Bitmap);
                            Event.Flags.onWindowchange = true;
                            Surface.Size.y = Height;
                            Surface.Size.x = Width;
                            DeleteObject(Old);
                        }

                        break;
                    }
                    case WM_MOVE:
                    {
                        const uint16_t X = LOWORD(Message.lParam);
                        const uint16_t Y = HIWORD(Message.lParam);

                        if (X != Surface.Position.x || Y != Surface.Position.y)
                        {
                            Event.Flags.onWindowchange = true;
                            Surface.Position.y = Y;
                            Surface.Position.x = X;
                        }

                        break;
                    }

                    // Special keys.
                    case WM_SYSKEYDOWN:
                    case WM_SYSKEYUP:
                    case WM_KEYDOWN:
                    case WM_KEYUP:
                    {
                        const bool Down = !((Message.lParam >> 31) & 1);
                        Event.Data.Keycode = Message.wParam;
                        Event.Flags.Keydown = Down;
                        Event.Flags.Keyup = !Down;

                        // Special cases.
                        switch (Message.wParam)
                        {
                            case VK_TAB: { Event.Flags.doTab = true; break; }
                            case VK_RETURN: { Event.Flags.doEnter = true; break; }
                            case VK_DELETE: { Event.Flags.doDelete = true; break; }
                            case VK_ESCAPE: { Event.Flags.doCancel = true; break; }
                            case VK_BACK: { Event.Flags.doBackspace = true; break; }
                            case 'X': { if (Shift) Event.Flags.doCut = true; break; }
                            case 'C': { if (Shift) Event.Flags.doCopy = true; break; }
                            case 'V': { if (Shift) Event.Flags.doPaste = true; break; }
                            case 'Z':
                            {
                                if (Ctrl)
                                {
                                    if (Shift) Event.Flags.doRedo = true;
                                    else Event.Flags.doUndo = true;
                                }
                                break;
                            }
                            default: break;
                        }

                        break;
                    }

                    // Keyboard input.
                    case WM_CHAR:
                    {
                        Event.Data.Letter = static_cast<wchar_t>(Message.wParam);
                        Event.Flags.Keydown = !((Message.lParam >> 31) & 1);
                        break;
                    }

                    // Mouse input.
                    case WM_LBUTTONUP:
                    case WM_RBUTTONUP:
                    case WM_LBUTTONDOWN:
                    case WM_RBUTTONDOWN:
                    {
                        Event.Data.Point = { LOWORD(Message.lParam), HIWORD(Message.lParam) };
                        Event.Flags.Mouseright  |= Message.message == WM_RBUTTONDOWN;
                        Event.Flags.Mouseleft  |= Message.message == WM_LBUTTONDOWN;
                        Event.Flags.Mousedown |= Message.message == WM_LBUTTONDOWN;
                        Event.Flags.Mousedown |= Message.message == WM_RBUTTONDOWN;
                        Event.Flags.Mouseright  |= Message.message == WM_RBUTTONUP;
                        Event.Flags.Mouseleft  |= Message.message == WM_LBUTTONUP;
                        Event.Flags.Mouseup = !Event.Flags.Mousedown;

                        if (Event.Flags.Mouseup) ReleaseCapture();
                        else SetCapture(Message.hwnd);
                        break;
                    }
                    case WM_MOUSEMOVE:
                    {
                        Event.Data.Point = { LOWORD(Message.lParam), HIWORD(Message.lParam) };
                        Event.Flags.Mousemove = true;
                        break;
                    }

                    default:
                    {
                        DispatchMessageA(&Message);
                        break;
                    }
                }

                // Notify the surface.
                if (Event.Flags.Raw) Surface.onEvent(&Surface, Event);
            }
        }
    }
    void Processsurfaces()
    {
        // Track the frame-time, should be less than 33ms.
        const auto Thisframe{ std::chrono::high_resolution_clock::now() };
        static auto Lastframe{ std::chrono::high_resolution_clock::now() };
        const auto Deltatime = std::chrono::duration<float>(Thisframe - Lastframe).count();

        // Get all events.
        Lastframe = Thisframe;
        Processinput();

        // Notify all timers.
        for(auto &[Name, Surface] : Surfaces) Surface.onFrame(&Surface, Deltatime);

        // Paint the surfaces.
        for (auto &[Name, Surface] : Surfaces)
        {
            // Only render when a surface is visible.
            if (IsWindowVisible(Surface.Windowhandle))
            {
                // Clean the context.
                SelectObject(Surface.Devicecontext, GetStockObject(DC_PEN));
                SelectObject(Surface.Devicecontext, GetStockObject(DC_BRUSH));

                // NOTE(tcn): This seems to be the fastest way to clear.
                SetBkColor(Surface.Devicecontext, Clearcolor);
                const RECT Screen = { 0, 0, Surface.Size.x, Surface.Size.y };
                ExtTextOutW(Surface.Devicecontext, 0, 0, ETO_OPAQUE, &Screen, NULL, 0, NULL);

                // Paint the surface.
                Surface.onRender(&Surface);

                // Paint the screen.
                BitBlt(GetDC(Surface.Windowhandle), 0, 0, Surface.Size.x, Surface.Size.y, Surface.Devicecontext, 0, 0, SRCCOPY);
            }
        }
    }
}

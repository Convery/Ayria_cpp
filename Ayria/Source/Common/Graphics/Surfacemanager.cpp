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
    constexpr auto Clearcolor = toColor(0xFF, 0xFF, 0xFF, 0xFF);
    const HBRUSH Clearbrush = CreateSolidBrush(Clearcolor);

    Surface_t *Createsurface(std::string_view Identifier)
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

        // Default initialization.
        Surface_t Surface{};
        Surface.Windowhandle = Windowhandle;
        Surface.Devicecontext = CreateCompatibleDC(GetDC(Windowhandle));

        // Only insert when ready.
        auto Entry = &Surfaces[Identifier.data()];
        *Entry = std::move(Surface);
        return Entry;
    }

    inline void Processinput()
    {
        POINT Globalmouse{};
        GetCursorPos(&Globalmouse);

        for (auto &[Name, Surface] : Surfaces)
        {
            Surface.Mouseposition = { Globalmouse.x - Surface.Position.x, Globalmouse.y - Surface.Position.y };
            const bool Ctrl = GetKeyState(VK_CONTROL) & (1 << 15);
            const bool Shift = GetKeyState(VK_SHIFT) & (1 << 15);

            MSG Message;
            while (PeekMessageA(&Message, Surface.Windowhandle, NULL, NULL, PM_REMOVE))
            {
                TranslateMessage(&Message);
                Event_t Event{};

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
                    case WM_KEYDOWN:
                    case WM_KEYUP:
                    case WM_SYSKEYDOWN:
                    case WM_SYSKEYUP:
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
                        }

                        break;
                    }

                    // Keyboard input.
                    case WM_CHAR:
                    {
                        const bool Down = !((Message.lParam >> 31) & 1);
                        Event.Data.Letter = Message.wParam;
                        Event.Flags.Keydown = Down;
                        break;
                    }

                    // Mouse input.
                    case WM_LBUTTONUP:
                    case WM_RBUTTONUP:
                    case WM_LBUTTONDOWN:
                    case WM_RBUTTONDOWN:
                    {
                        Event.Data.Point = { LOWORD(Message.lParam), HIWORD(Message.lParam) };
                        Event.Flags.Mousedown |= Message.message == WM_LBUTTONDOWN;
                        Event.Flags.Mousedown |= Message.message == WM_RBUTTONDOWN;
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

                // Notify the elements.
                if (Event.Flags.Raw)
                {
                    for (auto &Item : Surface.Elements)
                    {
                        if (Item.Wantedevents & Event.Flags.Raw)
                        {
                            Item.onEvent(&Surface, Event);
                        }
                    }
                }
            }
        }
    }
    void doFrame()
    {
        // Track the frame-time, should be less than 33ms.
        const auto Thisframe{ std::chrono::high_resolution_clock::now() };
        static auto Lastframe{ std::chrono::high_resolution_clock::now() };
        const auto Deltatime = std::chrono::duration<float>(Thisframe - Lastframe).count();

        // Get all events.
        Lastframe = Thisframe;
        Processinput();

        // Notify all timers.
        for(auto &[Name, Surface] : Surfaces)
        {
            for(auto &Element : Surface.Elements)
            {
                Element.onFrame(&Surface, Deltatime);
            }
        }

        // Paint the surfaces.
        for (auto &[Name, Surface] : Surfaces)
        {
            // Clean the context.
            SelectObject(Surface.Devicecontext, GetStockObject(DC_PEN));
            SelectObject(Surface.Devicecontext, GetStockObject(DC_BRUSH));

            // NOTE(tcn): This seems to be the fastest way to clear.
            SetBkColor(Surface.Devicecontext, Clearcolor);
            const RECT Screen = { 0, 0, Surface.Size.x, Surface.Size.y };
            ExtTextOutW(Surface.Devicecontext, 0, 0, ETO_OPAQUE, &Screen, NULL, 0, NULL);

            // Paint the surface.
            for (auto &Element : Surface.Elements)
            {
                // The method decides if the device needs updating.
                Element.onRender(&Surface);

                // Transparency through masking.
                if (Element.Mask)
                {
                    const auto Device = CreateCompatibleDC(Element.Devicecontext);
                    SelectObject(Device, Element.Mask);

                    BitBlt(Surface.Devicecontext, Element.Position.x, Element.Position.y, Element.Size.x, Element.Size.y, Device, 0, 0, SRCAND);
                    BitBlt(Surface.Devicecontext, Element.Position.x, Element.Position.y, Element.Size.x, Element.Size.y, Element.Devicecontext, 0, 0, SRCPAINT);

                    DeleteDC(Device);
                }
                else
                {
                    BitBlt(Surface.Devicecontext, Element.Position.x, Element.Position.y, Element.Size.x, Element.Size.y, Element.Devicecontext, 0, 0, SRCCOPY);
                }
            }

            // Paint the screen.
            BitBlt(GetDC(Surface.Windowhandle), 0, 0, Surface.Size.x, Surface.Size.y, Surface.Devicecontext, 0, 0, SRCCOPY);

            // Ensure that the surface is visible.
            ShowWindowAsync(Surface.Windowhandle, SW_SHOWNORMAL);
        }
    }
}

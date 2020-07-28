/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-07-22
    License: MIT
*/

#include "Graphics.hpp"
using namespace gInternal;

constexpr auto Clearcolor = toColorref(0xFF, 0xFF, 0xFF, 0xFF);
const HBRUSH Clearbrush = CreateSolidBrush(Clearcolor);

void Overlay_t::doFrame()
{
    // Track the frame-time, should be less than 33ms.
    const auto Thisframe{ std::chrono::high_resolution_clock::now() };
    static auto Lastframe{ std::chrono::high_resolution_clock::now() };
    const auto Deltatime = std::chrono::duration<float>(Thisframe - Lastframe).count();
    Lastframe = Thisframe;

    // Process events.
    {
        POINT Globalmouse{}; GetCursorPos(&Globalmouse);
        const bool Shift = GetKeyState(VK_SHIFT) & (1 << 15);
        const bool Ctrl = GetKeyState(VK_CONTROL) & (1 << 15);

        MSG Message;
        while (PeekMessageA(&Message, Windowhandle, NULL, NULL, PM_REMOVE))
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

                    if (Width != Size.x || Height != Size.y)
                    {
                        const auto Bitmap = CreateCompatibleBitmap(GetDC(Message.hwnd), Width, Height);
                        const auto Old = SelectObject(Devicecontext, Bitmap);
                        Event.Flags.onWindowchange = true;
                        DeleteObject(Old);
                        Size.y = Height;
                        Size.x = Width;
                    }

                    break;
                }
                case WM_MOVE:
                {
                    const uint16_t X = LOWORD(Message.lParam);
                    const uint16_t Y = HIWORD(Message.lParam);

                    if (X != Position.x || Y != Position.y)
                    {
                        Event.Flags.onWindowchange = true;
                        Position.y = Y;
                        Position.x = X;
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

            // Notify the callbacks.
            if (Event.Flags.Raw)
            {
                for (const auto &[Flags, Callback] : Eventcallbacks)
                {
                    if (Flags.Raw & Event.Flags.Raw)
                    {
                        Callback(Event);
                    }
                }
            }
        }
    }

    // Only render when the overlay is visible.
    if (IsWindowVisible(Windowhandle))
    {
        // Clean the overlay.
        SetBkColor(Devicecontext, Clearcolor);
        const RECT Screen = { 0, 0, Size.x, Size.y };
        SelectObject(Devicecontext, GetStockObject(DC_PEN));
        SelectObject(Devicecontext, GetStockObject(DC_BRUSH));

        // TODO(tcn): Investigate if there's a better way than to clean the whole surface at high-res.
        ExtTextOutW(Devicecontext, 0, 0, ETO_OPAQUE, &Screen, NULL, 0, NULL);

        // Paint the overlay.
        for (const auto &Surface : Surfaces)
        {
            // Transparency mask available.
            if (Surface.Mask) [[unlikely]]
            {
                BitBlt(Devicecontext, Surface.Position.x, Surface.Position.y,
                    Surface.Size.x, Surface.Size.y, Surface.Mask, 0, 0, SRCAND);
                BitBlt(Devicecontext, Surface.Position.x, Surface.Position.y,
                    Surface.Size.x, Surface.Size.y, Surface.Devicecontext, 0, 0, SRCPAINT);
            }
            else
            {
                BitBlt(Devicecontext, Surface.Position.x, Surface.Position.y,
                    Surface.Size.x, Surface.Size.y, Surface.Devicecontext, 0, 0, SRCCOPY);
            }
        }

        // Paint the screen.
        BitBlt(GetDC(Windowhandle), 0, 0, Size.x, Size.y, Devicecontext, 0, 0, SRCCOPY);
    }
}
void Overlay_t::Updatesurface(Surface_t &Template)
{
    auto Surface = std::find_if(Surfaces.begin(), Surfaces.end(), [&](const auto &Item)
    {
        return Template.Devicecontext == Item.Devicecontext;
    });
    assert(Surface != Surfaces.end());

    const auto isMoved = Template.Position.z != Surface->Position.z;
    *Surface = Template;

    // Need to re-sort.
    if (isMoved)
    {
        std::sort(Surfaces.begin(), Surfaces.end(), [](const auto &A, const auto &B)
        {
            return A.Position.z > B.Position.z;
        });
        std::stable_sort(Surfaces.begin(), Surfaces.end(), [](const auto &A, const auto &B)
        {
            return A.Position.x > B.Position.x;
        });
        std::stable_sort(Surfaces.begin(), Surfaces.end(), [](const auto &A, const auto &B)
        {
            return A.Position.y > B.Position.y;
        });
    }
}
Overlay_t::Overlay_t(point2_t Windowsize, point2_t Windowposition)
{
    // Register the window.
    WNDCLASSEXA Windowclass{};
    Windowclass.hbrBackground = Clearbrush;
    Windowclass.cbSize = sizeof(WNDCLASSEXA);
    Windowclass.lpfnWndProc = DefWindowProcW;
    Windowclass.lpszClassName = "Ayria_overlay";
    Windowclass.hInstance = GetModuleHandleA(NULL);
    Windowclass.style = CS_SAVEBITS | CS_BYTEALIGNWINDOW | CS_BYTEALIGNCLIENT | CS_OWNDC;
    if (NULL == RegisterClassExA(&Windowclass)) assert(false); // WTF?

    // Topmost, optionally transparent, no icon on the taskbar.
    Windowhandle = CreateWindowExA(WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TOPMOST, Windowclass.lpszClassName,
        NULL, WS_POPUP, NULL, NULL, NULL, NULL, NULL, NULL, Windowclass.hInstance, NULL);
    if (!Windowhandle) assert(false); // WTF?

    // Use a pixel-value to mean transparent rather than Alpha, because using Alpha is slow.
    SetLayeredWindowAttributes(Windowhandle, Clearcolor, 0, LWA_COLORKEY);

    // Fix the window.
    Size = Windowsize;
    Position = Windowposition;
    Devicecontext = CreateCompatibleDC(GetDC(Windowhandle));
    SetWindowPos(Windowhandle, NULL, Position.x, Position.y, Size.x, Size.y, NULL);
}
Surface_t Overlay_t::Createsurface(point2_t Size, point3_t Position, Eventpack_t Eventinfo)
{
    Surface_t Surface{ Position, Size, CreateCompatibleDC(Devicecontext) };
    Eventcallbacks.push_back(Eventinfo);
    Surfaces.push_back(Surface);
    Updatesurface(Surface);
    return Surface;
}

// Find windows not associated with our threads.
std::vector<std::pair<HWND, point2_t>> Graphics::Findwindows()
{
    std::vector<std::pair<HWND, point2_t>> Results;

    EnumWindows([](HWND Handle, LPARAM pResults) -> BOOL
    {
        DWORD ProcessID;
        const auto ThreadID = GetWindowThreadProcessId(Handle, &ProcessID);

        if (ProcessID == GetCurrentProcessId() && ThreadID != GetCurrentThreadId())
        {
            RECT Window{};
            if (GetWindowRect(Handle, &Window))
            {
                auto Results = (std::vector<std::pair<HWND, point2_t>> *)pResults;
                Results->push_back(
                    { Handle, { int16_t(Window.right - Window.left), int16_t(Window.bottom - Window.top) } });
            }
        }

        return TRUE;
    }, (LPARAM)&Results);

    return Results;
}

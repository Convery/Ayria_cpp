/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-08-12
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>
#include <Global.hpp>

constexpr COLORREF Clearcolor{ 0xFFFFFFFF };

inline HDC Createsurface(vec2_t Size, HDC Parent)
{
    const auto Context = CreateCompatibleDC(Parent);
    SelectObject(Context, CreateCompatibleBitmap(Context, Size.x, Size.y));
    return Context;
}
inline HDC Recreatesurface(vec2_t Size, HDC Context)
{
    auto Bitmap = CreateCompatibleBitmap(Context, Size.x, Size.y);
    DeleteObject(SelectObject(Context, Bitmap));
    return Context;
}

// Core building blocks.
#pragma region Elements
#pragma pack(push, 1)

using Eventflags_t = union
{
    union
    {
        uint32_t Raw;
        uint32_t Any;
    };
    struct
    {
        uint32_t
            onWindowchange : 1,
            onCharinput : 1,

            doBackspace : 1,
            doDelete : 1,
            doCancel : 1,
            doPaste : 1,
            doEnter : 1,
            doUndo : 1,
            doRedo : 1,
            doCopy : 1,
            doTab : 1,
            doCut : 1,

            Mousemove : 1,
            Mousedown : 1,
            Mouseup : 1,
            Keydown : 1,
            Keyup : 1,

            modShift : 1,
            modCtrl : 1,

            FLAG_MAX : 1;
    };
};
struct Event_t
{
    Eventflags_t Flags;
    union
    {
        uint32_t Keycode{};
        wchar_t Character;
        vec2_t Point;
    } Data;
};

struct Element_t
{
    Eventflags_t Wantedevents;
    vec2_t Position, Size;
    HDC Devicecontext;
    COLORREF Mask;
    bfloat16_t Z;

    void(__cdecl *onTick)(float Deltatime);
    void(__cdecl *onEvent)(Event_t Event);
};
struct Overlay_t
{
    std::chrono::high_resolution_clock::time_point Lastframe;
    std::vector<Element_t> Elements;
    vec2_t Position, Size;
    bool Ctrl{}, Shift{};
    HWND Windowhandle;

    void setWindowposition(vec2_t XY, bool Delta = false)
    {
        if (Delta) Position += XY;
        else Position = XY;

        SetWindowPos(Windowhandle, NULL, Position.x, Position.y, Size.x, Size.y, SWP_ASYNCWINDOWPOS | SWP_NOSIZE);
    }
    void setWindowsize(vec2_t WH, bool Delta = false)
    {
        if (Delta) Size += WH;
        else Size = WH;

        SetWindowPos(Windowhandle, NULL, Position.x, Position.y, Size.x, Size.y, SWP_ASYNCWINDOWPOS | SWP_NOMOVE);
    }
    void setVisible(bool Visible = true)
    {
        ShowWindowAsync(Windowhandle, Visible ? SW_SHOWNOACTIVATE : SW_HIDE);
    }
    void addElement(Element_t &&Element)
    {
        Element.Devicecontext = Createsurface(Element.Size, GetDC(Windowhandle));
        Elements.emplace_back(Element);

        // Sort by Z-order so we draw in the right order.
        std::sort(std::execution::par_unseq, Elements.begin(), Elements.end(), [](const auto &a, const auto &b)
        {
            return a.Z > b.Z;
        });
    }
    void doPresent()
    {
        // Only render when the overlay is visible.
        if (IsWindowVisible(Windowhandle))
        {
            const auto Devicecontext{ GetDC(Windowhandle) };
            const RECT Screen{ 0, 0, Size.x, Size.y };

            // Clean the context.
            SetBkColor(Devicecontext, Clearcolor);
            SelectObject(Devicecontext, GetStockObject(DC_PEN));
            SelectObject(Devicecontext, GetStockObject(DC_BRUSH));

            // TODO(tcn): Investigate if there's a better way than to clean the whole surface at high-resolution.
            ExtTextOutW(Devicecontext, 0, 0, ETO_OPAQUE, &Screen, NULL, 0, NULL);

            // Paint the overlay.
            for (const auto &Element : Elements)
            {
                // Transparency mask available.
                if (Element.Mask) [[unlikely]]
                {
                    // TODO(tcn): Benchmark against using Createmask() with BitBlt(SRCAND) + BitBlt(SRCPAINT).
                    TransparentBlt(Devicecontext, Element.Position.x, Element.Position.y,
                        Element.Size.x, Element.Size.y, Element.Devicecontext, 0, 0,
                        Element.Size.x, Element.Size.y, Element.Mask);
                }
                else
                {
                    BitBlt(Devicecontext, Element.Position.x, Element.Position.y,
                        Element.Size.x, Element.Size.y, Element.Devicecontext, 0, 0, SRCCOPY);
                }
            }

            // Paint the window and notify Windows that it's clean.
            BitBlt(GetDC(Windowhandle), Position.x, Position.y, Size.x, Size.y, Devicecontext, 0, 0, SRCCOPY);
            ValidateRect(Windowhandle, NULL);
        }
    }
    void doFrame()
    {
        // Process events.
        {
            MSG Message;
            while (PeekMessageA(&Message, Windowhandle, NULL, NULL, PM_REMOVE))
            {
                Event_t Event{};

                TranslateMessage(&Message);
                switch (Message.message)
                {
                    // Common case.
                    case WM_PAINT:
                    {
                        doPresent();
                        break;
                    }

                    // Window-changes.
                    case WM_SIZE:
                    {
                        const vec2_t Newsize{ LOWORD(Message.lParam), HIWORD(Message.lParam) };

                        if (Size != Newsize)
                        {
                            Recreatesurface(Newsize, GetDC(Windowhandle));
                            Event.Flags.onWindowchange = true;
                            Event.Data.Point = Newsize;
                            Size = Newsize;
                        }

                        break;
                    }
                    case WM_MOVE:
                    {
                        Position = { LOWORD(Message.lParam), HIWORD(Message.lParam) };
                        break;
                    }

                    // Special keys.
                    case WM_SYSKEYDOWN:
                    case WM_SYSKEYUP:
                    case WM_KEYDOWN:
                    case WM_KEYUP:
                    {
                        const bool Down = !((Message.lParam >> 31) & 1);

                        // Special cases.
                        switch (Message.wParam)
                        {
                            case VK_SHIFT: { Shift = Down; break; }
                            case VK_CONTROL: { Ctrl = Down; break; }

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

                        if (Event.Flags.Any)
                        {
                            Event.Data.Keycode = Message.wParam;
                            Event.Flags.modShift = Shift;
                            Event.Flags.modCtrl = Ctrl;
                            Event.Flags.Keydown = Down;
                            Event.Flags.Keyup = !Down;
                        }
                        break;
                    }

                    // Keyboard input.
                    case WM_CHAR:
                    {
                        Event.Data.Character = static_cast<wchar_t>(Message.wParam);
                        Event.Flags.onCharinput = true;
                        Event.Flags.modShift = Shift;
                        Event.Flags.modCtrl = Ctrl;
                        break;
                    }

                    // Mouse input.
                    case WM_LBUTTONUP:
                    case WM_RBUTTONUP:
                    case WM_MBUTTONUP:
                    case WM_LBUTTONDOWN:
                    case WM_RBUTTONDOWN:
                    case WM_MBUTTONDOWN:
                    {
                        const bool Middle = Message.message == WM_MBUTTONDOWN || Message.message == WM_MBUTTONUP;
                        const bool Right = Message.message == WM_RBUTTONDOWN || Message.message == WM_RBUTTONUP;
                        const bool Left = Message.message == WM_LBUTTONDOWN || Message.message == WM_LBUTTONUP;
                        if (Middle) Event.Data.Keycode = VK_MBUTTON;
                        if (Right) Event.Data.Keycode = VK_RBUTTON;
                        if (Left) Event.Data.Keycode = VK_LBUTTON;

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

                    default: continue;
                }

                // Notify the elements.
                std::for_each(std::execution::par_unseq, Elements.begin(), Elements.end(), [=](const Element_t &Element)
                {
                    if (Element.onEvent && (Element.Wantedevents.Raw & Event.Flags.Raw)) Element.onEvent(Event);
                });
            }
        }

        // Track the frame-time, should be less than 33ms.
        const auto Thisframe{ std::chrono::high_resolution_clock::now() };
        const auto Deltatime = std::chrono::duration<float>(Thisframe - Lastframe).count();
        Lastframe = Thisframe;

        // Log frame-average every 5 seconds.
        if constexpr (Build::isDebug)
        {
            static std::array<float, 256> Timings{};
            static float Elapsedtime{};
            static size_t Index{};

            Timings[Index % 256] = Deltatime;
            Elapsedtime += Deltatime;
            Index++;

            if (Elapsedtime >= 5.0f)
            {
                const auto Sum = std::reduce(std::execution::par_unseq, Timings.begin(), Timings.end());
                Debugprint(va("Average frametime: %f", Sum / 256.0f));
                Elapsedtime = 0;
            }
        }

        // Notify the elements.
        std::for_each(std::execution::par_unseq, Elements.begin(), Elements.end(), [=](const Element_t &Element)
        {
            if (Element.onTick) Element.onTick(Deltatime);
        });
    }

    explicit Overlay_t(vec2_t XY, vec2_t WH) : Position(XY), Size(WH)
    {
        // Register the window.
        WNDCLASSEXA Windowclass{ sizeof(WNDCLASSEXA) };
        Windowclass.lpszClassName = "Ayria_UI_overlay";
        Windowclass.hInstance = GetModuleHandleA(NULL);
        Windowclass.style = CS_SAVEBITS | CS_BYTEALIGNWINDOW | CS_BYTEALIGNCLIENT | CS_OWNDC;
        Windowclass.lpfnWndProc = [](HWND a, UINT b, WPARAM c, LPARAM d)
        {
            if (b == WM_PAINT) return LRESULT(1);
            if (b == WM_SIZE || b == WM_MOVE)
                PostMessageA(a, b, c, d);

            return DefWindowProcA(a, b, c, d);
        };
        if (NULL == RegisterClassExA(&Windowclass)) assert(false); // WTF?

        // Topmost, optionally transparent, no icon on the taskbar, zero size so it's not shown.
        Windowhandle = CreateWindowExA(WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TOPMOST, Windowclass.lpszClassName,
            NULL, WS_POPUP, Position.x, Position.y, NULL, NULL, NULL, NULL, Windowclass.hInstance, NULL);
        if (!Windowhandle) assert(false); // WTF?

        // Use a pixel-value to mean transparent rather than Alpha, because using Alpha is slow.
        SetLayeredWindowAttributes(Windowhandle, Clearcolor, 0, LWA_COLORKEY);

        // If we got a size, resize and show.
        if (Size) setWindowsize(Size);
    }
};

#pragma pack(pop)
#pragma endregion

// TODO(tcn): Add image loading from disk.
inline HBITMAP Createimage(vec2_t Dimensions, size_t Size, const void *Data)
{
    assert(Dimensions.x); assert(Dimensions.y); assert(Size); assert(Data);
    const size_t Pixelsize{ Size / int(Dimensions.x * Dimensions.y) };
    BITMAPINFO BMI{ sizeof(BITMAPINFOHEADER) };
    uint8_t *DIBPixels{};

    // Windows likes to draw bitmaps upside down, so negative height.
    BMI.bmiHeader.biSizeImage = int(Dimensions.y * Dimensions.x) * Pixelsize;
    BMI.bmiHeader.biHeight = (-1) * int(Dimensions.y);
    BMI.bmiHeader.biBitCount = short(Pixelsize * 8);
    BMI.bmiHeader.biWidth = Dimensions.x;
    BMI.bmiHeader.biCompression = BI_RGB;
    BMI.bmiHeader.biPlanes = 1;

    // Allocate the bitmap in system-memory.
    const auto Bitmap = CreateDIBSection(NULL, &BMI, DIB_RGB_COLORS, (void **)&DIBPixels, NULL, 0);
    std::memcpy(DIBPixels, Data, Size);

    // Need to synchronize the bits with GDI.
    GdiFlush();
    {
        SetDIBits(NULL, Bitmap, 0, Dimensions.y, DIBPixels, &BMI, DIB_RGB_COLORS);
    }
    GdiFlush();

    return Bitmap;
}
inline HBITMAP Createmask(HDC Devicecontext, vec2_t Size, COLORREF Transparancykey)
{
    const auto Bitmap = CreateBitmap(int(Size.x), int(Size.y), 1, 1, NULL);
    const auto Device = CreateCompatibleDC(Devicecontext);

    SetBkColor(Devicecontext, Transparancykey);
    SelectObject(Device, Bitmap);

    BitBlt(Device, 0, 0, Size.x, Size.y, Devicecontext, 0, 0, SRCCOPY);
    BitBlt(Device, 0, 0, Size.x, Size.y, Device, 0, 0, SRCINVERT);

    GdiFlush();

    DeleteDC(Device);
    return Bitmap;
}

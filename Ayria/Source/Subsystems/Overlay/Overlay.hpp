/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-08-12
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>
#include <Global.hpp>
#include <variant>

inline HDC Createsurface(vec2_t Size, HDC Parent)
{
    const auto Context = CreateCompatibleDC(Parent);
    SelectObject(Context, CreateCompatibleBitmap(Parent, Size.x, Size.y));
    return Context;
}
inline HDC Recreatesurface(vec2_t Size, HDC Context)
{
    auto Bitmap = CreateCompatibleBitmap(Context, Size.x, Size.y);
    DeleteObject(SelectObject(Context, Bitmap));
    return Context;
}

// Core building blocks.
#pragma region Core
#pragma pack(push, 1)

struct Element_t
{
    HDC Surface;                // Bitmap to BitBlt and draw to, update Repainted on write.
    Eventflags_t Wantedevents;  // Matched in Overlay::Broadcastevent
    vec3_t Position;            // Z only used for rendering-order.
    bool Repainted;             // Should be atomic_flag, but we need to sort elements.
    COLORREF Mask;              // NULL = unused.
    vec2_t Size;

    void(__cdecl *onTick)(Element_t *This, float Deltatime);
    void(__cdecl *onEvent)(Element_t *This, Eventflags_t Flags, std::variant<uint32_t, vec2_t, wchar_t> Data);
};
struct Overlay_t
{
    std::vector<Element_t> Elements;
    vec2_t Position, Size;
    bool Ctrl{}, Shift{};
    bool Forcerepaint{};
    HWND Windowhandle;

    static LRESULT __stdcall Windowproc(HWND Windowhandle, UINT Message, WPARAM wParam, LPARAM lParam)
    {
        Overlay_t *This{};

        if (Message == WM_NCCREATE)
        {
            This = (Overlay_t *)((LPCREATESTRUCTA)lParam)->lpCreateParams;
            SetWindowLongPtr(Windowhandle, GWLP_USERDATA, (LONG_PTR)This);
            This->Windowhandle = Windowhandle;
        }
        else
        {
            This = (Overlay_t *)GetWindowLongPtrA(Windowhandle, GWLP_USERDATA);
        }

        if (This) return This->Eventhandler(Message, wParam, lParam);
        return DefWindowProcA(Windowhandle, Message, wParam, lParam);
    }
    void Broadcastevent(Eventflags_t Flags, std::variant<uint32_t, vec2_t, wchar_t> Data)
    {
        std::for_each(std::execution::par_unseq, Elements.begin(), Elements.end(), [=](Element_t &Element)
        {
            if (Element.Wantedevents.Raw & Flags.Raw) [[unlikely]]
            {
                if (Element.onEvent) [[likely]] Element.onEvent(&Element, Flags, Data);
            }
        });
    }
    LRESULT __stdcall Eventhandler(UINT Message, WPARAM wParam, LPARAM lParam)
    {
        Eventflags_t Flags{};

        switch (Message)
        {
            // Default Windows BS.
            case WM_MOUSEACTIVATE: return MA_NOACTIVATE;
            case WM_NCCREATE: return Windowhandle != NULL;

            // Preferred over WM_MOVE/WM_SIZE for performance.
            case WM_WINDOWPOSCHANGED:
            {
                const auto Info = (WINDOWPOS *)lParam;
                Position = { Info->x, Info->y };
                Size = { Info->cx, Info->cy };

                // Notify the elements that they need to resize.
                Flags.onWindowchange = true;
                Broadcastevent(Flags, Size);
                return NULL;
            }

            // Special keys.
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP:
            {
                const bool Down = !((lParam >> 31) & 1);

                switch (wParam)
                {
                    case VK_TAB: { Flags.doTab = !Down; break; }
                    case VK_RETURN: { Flags.doEnter = !Down; break; }
                    case VK_DELETE: { Flags.doDelete = !Down; break; }
                    case VK_ESCAPE: { Flags.doCancel = !Down; break; }
                    case VK_BACK: { Flags.doBackspace = !Down; break; }
                    case 'X': { if (Shift) Flags.doCut = !Down; break; }
                    case 'C': { if (Shift) Flags.doCopy = !Down; break; }
                    case 'V': { if (Shift) Flags.doPaste = !Down; break; }
                    case 'Z':
                    {
                        if (Ctrl)
                        {
                            if (Shift) Flags.doRedo = !Down;
                            else Flags.doUndo = !Down;
                        }
                        break;
                    }
                    default: break;
                }


                Flags.modShift = Shift;
                Flags.modCtrl = Ctrl;
                Flags.Keydown = Down;
                Flags.Keyup = !Down;

                Broadcastevent(Flags, uint32_t(wParam));
                return NULL;
            }

            // Keyboard input.
            case WM_UNICHAR:
            case WM_CHAR:
            {
                // Why would you send this as a char?!
                if (VK_BACK != wchar_t(wParam))
                {
                    Flags.onCharinput = true; Flags.modShift = Shift; Flags.modCtrl = Ctrl;
                    Broadcastevent(Flags, wchar_t(wParam));
                }
                return NULL;
            }

            // Mouse input.
            case WM_LBUTTONUP:
            case WM_RBUTTONUP:
            case WM_MBUTTONUP:
            case WM_LBUTTONDOWN:
            case WM_RBUTTONDOWN:
            case WM_MBUTTONDOWN:
            {
                if (Flags.Mouseup) ReleaseCapture();
                else SetCapture(Windowhandle);

                Flags.Mousemiddle = Message == WM_MBUTTONDOWN || Message == WM_MBUTTONUP;
                Flags.Mouseright = Message == WM_RBUTTONDOWN || Message == WM_RBUTTONUP;
                Flags.Mouseleft = Message == WM_LBUTTONDOWN || Message == WM_LBUTTONUP;
                Flags.Mousedown |= Message == WM_LBUTTONDOWN;
                Flags.Mousedown |= Message == WM_RBUTTONDOWN;
                Flags.Mousedown |= Message == WM_MBUTTONDOWN;
                Flags.Mouseup = !Flags.Mousedown;

                Broadcastevent(Flags, vec2_t(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)));
                return NULL;
            }
            case WM_MOUSEMOVE:
            {
                Flags.Mousemove = true;
                Broadcastevent(Flags, vec2_t(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)));
                return NULL;
            }

            // To keep tools happy.
            default: break;
        }

        return DefWindowProcA(Windowhandle, Message, wParam, lParam);
    }

    void setWindowposition(vec2_t XY, bool Delta = false)
    {
        if (Delta) XY += Position;
        SetWindowPos(Windowhandle, NULL, XY.x, XY.y, Size.x, Size.y, SWP_ASYNCWINDOWPOS | SWP_NOSIZE);
    }
    void setWindowsize(vec2_t WH, bool Delta = false)
    {
        if (Delta) WH += Size;
        SetWindowPos(Windowhandle, NULL, Position.x, Position.y, WH.x, WH.y, SWP_ASYNCWINDOWPOS | SWP_NOMOVE);
    }
    void setVisible(bool Visible = true)
    {
        ShowWindowAsync(Windowhandle, Visible ? SW_SHOW : SW_HIDE);
        Forcerepaint = true;
    }

    void addElement(Element_t &&Element)
    {
        Element.Surface = Createsurface(Element.Size);
        Elements.emplace_back(Element);

        // Sort by Z-order so we draw in the right order.
        std::sort(std::execution::par_unseq, Elements.begin(), Elements.end(), [](const auto &a, const auto &b)
        {
            return a.Position.z > b.Position.z;
        });

        // Sort by X and Y for slightly better cache locality in rendering.
        std::stable_sort(std::execution::par_unseq, Elements.begin(), Elements.end(), [](const auto &a, const auto &b)
        {
            return a.Position.x > b.Position.x;
        });
        std::stable_sort(std::execution::par_unseq, Elements.begin(), Elements.end(), [](const auto &a, const auto &b)
        {
            return a.Position.y > b.Position.y;
        });
    }
    HDC Createsurface(vec2_t eSize, HDC *Previous = nullptr)
    {
        const auto Device = GetDC(Windowhandle);
        const auto Context = Previous ? *Previous : CreateCompatibleDC(Device);
        DeleteObject(SelectObject(Context, CreateCompatibleBitmap(Device, eSize.x, eSize.y)));
        ReleaseDC(Windowhandle, Device);
        return Context;
    }

    void doFrame(float Deltatime)
    {
        // Process events.
        {
            // Reset the state between frames.
            Shift = GetKeyState(VK_SHIFT) & (1 << 15);
            Ctrl = GetKeyState(VK_CONTROL) & (1 << 15);

            MSG Message;
            while (PeekMessageA(&Message, Windowhandle, NULL, NULL, PM_REMOVE))
            {
                TranslateMessage(&Message);
                DispatchMessageA(&Message);
            }
        }

        // Notify the elements about the frame, needs to sync with GDI so Seq.
        std::for_each(std::execution::seq, Elements.begin(), Elements.end(), [=](auto &Element)
        {
            if (Element.onTick) [[unlikely]] Element.onTick(&Element, Deltatime);
        });

        // Repaint the overlay.
        doPresent();
    }
    void doPresent()
    {
        // Only render when the overlay is visible.
        if (IsWindowVisible(Windowhandle))
        {
            // Before doing any work, verify that we have to update.
            if (!Forcerepaint)
            {
                if (!std::any_of(std::execution::par_unseq, Elements.begin(), Elements.end(),
                    [](const auto &Element) { return Element.Repainted; })) return;
            }
            Forcerepaint = false;

            PAINTSTRUCT State;
            InvalidateRect(Windowhandle, NULL, FALSE);
            const auto Device = BeginPaint(Windowhandle, &State);

            // Ensure that the device is 'clean'.
            SelectObject(Device, GetStockObject(DC_PEN));
            SelectObject(Device, GetStockObject(DC_BRUSH));
            SelectObject(Device, GetStockObject(SYSTEM_FONT));

            // Paint the overlay, elements are ordered by Z.
            for (auto &Element : Elements)
            {
                // Rather than test, just clear.
                Element.Repainted = false;

                // Rare, an invisible element.
                if (!Element.Surface) [[unlikely]]
                    continue;

                // Transparency mask available.
                if (Element.Mask) [[unlikely]]
                {
                    // TODO(tcn): Benchmark against using Createmask() with BitBlt(SRCAND) + BitBlt(SRCPAINT).
                    TransparentBlt(Device, Element.Position.x, Element.Position.y,
                    Element.Size.x, Element.Size.y, Element.Surface, 0, 0,
                    Element.Size.x, Element.Size.y, Element.Mask);
                }
                else
                {
                    BitBlt(Device, Element.Position.x, Element.Position.y,
                        Element.Size.x, Element.Size.y, Element.Surface, 0, 0, SRCCOPY);
                }
            }

            // Cleanup.
            EndPaint(Windowhandle, &State);
        }
    }

    explicit Overlay_t(vec2_t _Position, vec2_t _Size) : Position(_Position)
    {
        // Register the overlay class.
        WNDCLASSEXA Windowclass{};
        Windowclass.lpfnWndProc = Windowproc;
        Windowclass.cbSize = sizeof(WNDCLASSEXA);
        Windowclass.lpszClassName = "Ayria_Overlay";
        Windowclass.cbWndExtra = sizeof(Overlay_t *);
        Windowclass.hbrBackground = CreateSolidBrush(Clearcolor);
        if (NULL == RegisterClassExA(&Windowclass)) assert(false);

        // Generic overlay style.
        const DWORD Style = WS_POPUP | (Build::isDebug * WS_BORDER);
        const DWORD StyleEx = WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED;

        // Topmost, optionally transparent, no icon on the taskbar, zero size so it's not shown.
        if (!CreateWindowExA(StyleEx, Windowclass.lpszClassName, NULL, Style, Position.x, Position.y,
            0, 0, NULL, NULL, NULL, this)) assert(false);

        // To keep static-analysis happy. Is set via Windowproc.
        assert(Windowhandle);

        // Use a pixel-value to mean transparent rather than Alpha, because using Alpha is slow.
        SetLayeredWindowAttributes(Windowhandle, Clearcolor, 0, LWA_COLORKEY);

        // If we have a size, resize to show the window.
        if (_Size) setWindowsize(_Size);
    }
};

#pragma pack(pop)
#pragma endregion

// TODO(tcn): Add font from disk / resource.
inline HFONT Createfont(std::string_view Name, int8_t Fontsize)
{
    return CreateFontA(Fontsize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, Name.data());
}
inline HFONT getDefaultfont()
{
    static const auto Default = Createfont("Consolas", 16);
    return Default;
}

// TODO(tcn): Add image loading from disk / resource.
inline HBITMAP Createimage(vec2_t Dimensions, uint32_t Size, const void *Data)
{
    assert(Dimensions.x); assert(Dimensions.y); assert(Size); assert(Data);
    const auto Pixelsize{ Size / int(Dimensions.x * Dimensions.y) };
    BITMAPINFO BMI{ sizeof(BITMAPINFOHEADER) };
    uint8_t *DIBPixels;

    // Windows likes to draw bitmaps upside down, so negative height.
    BMI.bmiHeader.biSizeImage = int(Dimensions.y * Dimensions.x) * Pixelsize;
    BMI.bmiHeader.biHeight = (-1) * int(Dimensions.y);
    BMI.bmiHeader.biBitCount = short(Pixelsize * 8);
    BMI.bmiHeader.biWidth = Dimensions.x;
    BMI.bmiHeader.biCompression = BI_RGB;
    BMI.bmiHeader.biPlanes = 1;

    // Allocate the bitmap in system-memory.
    const auto Bitmap = CreateDIBSection(NULL, &BMI, DIB_RGB_COLORS, (void **)&DIBPixels, NULL, 0);
    assert(Bitmap); assert(DIBPixels); // WTF?
    std::memcpy(DIBPixels, Data, Size);

    // NOTE(tcn): Probably need to synchronize the bits with GDI via GdiFlush().
    SetDIBits(NULL, Bitmap, 0, Dimensions.y, DIBPixels, &BMI, DIB_RGB_COLORS);

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

#if defined(HAS_VECTORCLASS)
// Windows sometimes want pixels as BGRA.
template <bool hasAlpha> void SwapRB(uint32_t Size, uint8_t *Pixeldata)
{
    #if !defined(__AVX__)
    #pragma message("AVX not enabled, unaligned access may cause poor performance in SwapRB().")
    #endif

    // We only use 120 bits per __m128 for RGB.
    constexpr auto Pixelsize = 3 + hasAlpha;
    constexpr auto Stride = 15 + hasAlpha;
    const auto Count128 = Size / Stride;

    // NOTE(tcn): MSVC has issues optimizing Size % Stride, so we do it manually.
    const auto Remaining = [=]()
    {
        if constexpr (hasAlpha) return Size & (Stride - 1);
        else
        {
            uint32_t Rem = 0;
            for (Rem = Size; Size > Stride; Size = Rem)
                for (Rem = 0; Size; Size >>= 4)
                    Rem += Size & Stride;
            return Rem == Stride ? 0 : Rem;
        }
    }();

    // Bulk operations.
    for (size_t i = 0; i < Count128; ++i)
    {
        vcl::Vec16uc Line;
        Line.load(Pixeldata + (Stride * i));

        // Line store should be optimized away.
        if constexpr (hasAlpha)
            Line = vcl::permute16<2, 1, 0, 3, 6, 5, 4, 7, 10, 9, 8, 11, 14, 13, 12, 15>(Line);
        else
            Line = vcl::permute16<2, 1, 0, 5, 4, 3, 8, 7, 6, 11, 10, 9, 14, 13, 12, 15>(Line);

        Line.store(Pixeldata + (Stride * i));
    }

    // Remainder.
    const auto Offset = Count128 * Stride;
    for (size_t i = 0; i < Remaining; ++i)
    {
        std::swap(Pixeldata[Offset + (i * Pixelsize)],
                  Pixeldata[Offset + (i * Pixelsize) + 2]);
    }
}
#endif

// Find windows not associated with our threads.
inline std::vector<std::pair<HWND, vec4_t>> Findwindows()
{
    std::vector<std::pair<HWND, vec4_t>> Results;

    EnumWindows([](HWND Handle, LPARAM pResults) -> BOOL
    {
        DWORD ProcessID;
        const auto ThreadID = GetWindowThreadProcessId(Handle, &ProcessID);

        if (ProcessID == GetCurrentProcessId() && ThreadID != GetCurrentThreadId())
        {
            RECT Window{};
            if (GetWindowRect(Handle, &Window))
            {
                const vec4_t Temp(Window.right, Window.left, Window.bottom, Window.top);
                auto Results = (std::vector<std::pair<HWND, vec4_t>> *)pResults;
                Results->push_back(std::make_pair(Handle, Temp));
            }
        }

        return TRUE;
    }, (LPARAM)&Results);

    return Results;
}
inline std::pair<HWND, vec4_t> Largestwindow()
{
    std::pair<HWND, vec4_t> Result;

    EnumWindows([](HWND Handle, LPARAM pResult) -> BOOL
    {
        DWORD ProcessID;
        const auto ThreadID = GetWindowThreadProcessId(Handle, &ProcessID);

        if (ProcessID == GetCurrentProcessId() && ThreadID != GetCurrentThreadId())
        {
            RECT Window{};
            if (GetWindowRect(Handle, &Window))
            {
                const vec4_t Temp(Window.right, Window.left, Window.bottom, Window.top);
                auto Result = (std::pair<HWND, vec4_t> *)pResult;

                if (Result->second > Temp) *Result = std::make_pair(Handle, Temp);
            }
        }

        return TRUE;
    }, (LPARAM)&Result);

    return Result;
}

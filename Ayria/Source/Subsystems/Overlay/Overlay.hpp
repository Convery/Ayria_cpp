/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-06-11
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>
#include <Global.hpp>

// VK 0x3A-0x40 are undefined on Windows, let's reuse them.
enum VK_EXTENDED
{
    VK_CUT = 0x3A,
    VK_COPY,
    VK_PASTE,
    VK_UNDO,
    VK_REDO,
};
using Eventflags_t = union
{
    uint8_t Raw;
    uint8_t Any;
    struct
    {
        uint8_t
            onWindowchange : 1,
            onMousemove : 1,
            onCharinput : 1,
            onClick : 1,
            onKey : 1,

            modShift : 1,
            modCtrl : 1,
            modDown : 1;
    };
};

// Callback types that the element can provide.
using Tickcallback  = void(__cdecl *)(class Overlay_t *Parent, uint32_t DeltatimeMS);
using Paintcallback = void(__cdecl *)(class Overlay_t *Parent, const Graphics::Renderer_t &Renderer);
using Eventcallback = void(__cdecl *)(class Overlay_t *Parent, Eventflags_t Eventtype, const std::variant<uint32_t, vec2i> &Eventdata);

// Forward declaration for the overlays.
class Overlay_t *Createconsoleoverlay();

// Core building block of the graphics.
struct Elementinfo_t
{
    vec2i Position{}, Size{};
    Paintcallback onPaint{};
    Eventcallback onEvent{};
    Tickcallback onTick{};

    Eventflags_t Eventmask{};
    int8_t ZIndex{};

    std::atomic_flag isFocused{};
};

// Window host and element-manager.
constexpr size_t Defaultcount = 4;
class Overlay_t
{
    protected:
    HWND Windowhandle{};
    Spinlock Threadlock{};
    std::atomic_flag dirtyCache{};
    std::atomic_flag dirtyBuffer{};
    std::atomic<RECT> dirtyScreen{ { 9999, 9999, 0, 0 } };

    // Cache of the elements properties by Z.
    Inlinedvector<int8, Defaultcount> ZIndex{};
    Inlinedvector<vec4i, Defaultcount> Hitbox{};
    Inlinedvector<uint8, Defaultcount> Eventmasks{};
    Inlinedvector<Tickcallback, Defaultcount> Tickcallbacks{};
    Inlinedvector<Eventcallback, Defaultcount> Eventcallbacks{};
    Inlinedvector<Paintcallback, Defaultcount> Paintcallbacks{};
    Inlinedvector<Elementinfo_t *, Defaultcount> Trackedelements{};

    // Callbacks that could be extrended.
    virtual void onPaint()
    {
        const RECT Rect = dirtyScreen.load();
        dirtyScreen.store({});
        dirtyBuffer.clear();

        const vec4i Paintrect(Rect.left, Rect.top, Rect.right, Rect.bottom);
        const vec2i Size(Paintrect.z - Paintrect.x, Paintrect.w - Paintrect.y);

        const auto Windowcontext = GetDC(Windowhandle);
        const auto Devicecontext = CreateCompatibleDC(Windowcontext);
        const auto BMP = CreateCompatibleBitmap(Windowcontext, Windowsize.x, Windowsize.y);
        DeleteObject(SelectObject(Devicecontext, BMP));

        // Clipped rendering to the rect that needs updating.
        const Graphics::Renderer_t Renderer(Devicecontext, Paintrect);
        BitBlt(Devicecontext, Paintrect.x, Paintrect.y, Size.x, Size.y, NULL, 0, 0, WHITENESS);

        // Drawing needs to be sequential, skip elements that are out of view.
        const auto Range = lz::zip(Paintcallbacks, Hitbox);
        for (const auto &[Callback, Box] : Range)
        {
            if (Box.x > Paintrect.z || Box.z < Paintrect.x) [[unlikely]] continue;
            if (Box.y > Paintrect.w || Box.w < Paintrect.y) [[unlikely]] continue;
            if (!Callback) [[unlikely]] continue;
            Callback(this, Renderer);
        }

        BitBlt(Windowcontext, Paintrect.x, Paintrect.y, Size.x, Size.y, Devicecontext, Paintrect.x, Paintrect.y, SRCCOPY);

        DeleteObject(BMP);
        DeleteDC(Devicecontext);
        ReleaseDC(Windowhandle, Windowcontext);

        ValidateRect(Windowhandle, NULL);
    }
    virtual void onTick(uint32_t DeltatimeMS)
    {
        std::for_each(std::execution::par_unseq, Tickcallbacks.cbegin(), Tickcallbacks.cend(),
            [&](auto &Callback) { if (Callback) [[likely]] Callback(this, DeltatimeMS); });
    }
    virtual void onEvent(Eventflags_t Eventtype, std::variant<uint32_t, vec2i> Eventdata)
    {
        // Basic hit detection.
        if (Eventtype.onMousemove)
        {
            const auto &Mouseposition = std::get<vec2i>(Eventdata);
            const auto Range = lz::zip(Trackedelements, Hitbox);

            std::for_each(std::execution::par_unseq, Range.begin(), Range.end(), [&](const auto &Tuple)
            {
                const auto &[Element, Box] = Tuple;

                if (Mouseposition.x >= Box.x && Mouseposition.x <= Box.z &&
                    Mouseposition.y >= Box.y && Mouseposition.y <= Box.w)
                {
                    Element->isFocused.test_and_set();
                }
                else
                {
                    Element->isFocused.clear();
                }
            });
        }

        // We do not care about the order of elements here.
        const auto Range = lz::zip(Eventcallbacks, Eventmasks);
        std::for_each(std::execution::par_unseq, Range.begin(), Range.end(), [&](const auto &Tuple)
        {
            const auto &[Callback, Mask] = Tuple;

            if ((Eventtype.Raw & Mask) && Callback) [[likely]]
            {
                Callback(this, Eventtype, Eventdata);
            }
        });
    }

    public:
    vec2i Windowsize{};
    vec2i Windowposition{};

    // Access from the elements.
    void Invalidateelements()
    {
        dirtyCache.test_and_set();
    }
    void Insertelement(Elementinfo_t *Element)
    {
        std::scoped_lock Guard(Threadlock);
        assert(Element);

        Trackedelements.push_back(Element);
        ZIndex.emplace_back(Element->ZIndex);
        Tickcallbacks.emplace_back(Element->onTick);
        Eventcallbacks.emplace_back(Element->onEvent);
        Paintcallbacks.emplace_back(Element->onPaint);
        Eventmasks.emplace_back(Element->Eventmask.Raw);
        Hitbox.emplace_back(Element->Position.x, Element->Position.y,
                            Element->Position.x + Element->Size.x, Element->Position.y + Element->Size.y);

        dirtyCache.test_and_set();
    }
    void Invalidatescreen(vec4i &&Dirtyrect)
    {
        auto Original = dirtyScreen.load();
        if (0 == std::memcmp(&Original, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 16))
        {
            dirtyScreen.store(Dirtyrect);
        }
        else
        {
            while (true)
            {

                Original = dirtyScreen.load();
                auto TMP = Original;
                TMP.top = std::min(TMP.top, (LONG)Dirtyrect.y);
                TMP.left = std::min(TMP.left, (LONG)Dirtyrect.x);
                TMP.right = std::max(TMP.right, (LONG)Dirtyrect.z);
                TMP.bottom = std::max(TMP.bottom, (LONG)Dirtyrect.w);
                if (dirtyScreen.compare_exchange_weak(Original, TMP)) break;
            }
        }

        dirtyBuffer.test_and_set();
    }
    void Invalidatescreen(vec2i Dirtypos, vec2i Dirtysize)
    {
        return Invalidatescreen(vec4i{ Dirtypos.x, Dirtypos.y, Dirtypos.x + Dirtysize.x, Dirtypos.y + Dirtysize.y });
    }

    // Called each frame, dispatches to derived classes.
    void onFrame(float Deltatime)
    {
        const uint32_t DeltatimeMS = Deltatime * 1000.0f;

        // If the cache is dirty, we need to sort it again.
        if (dirtyCache.test()) [[unlikely]]
        {
            std::scoped_lock Guard(Threadlock);
            dirtyCache.clear();

            // Re-fetch all the data.
            for (size_t i = 0; i < Trackedelements.size(); ++i)
            {
                const auto Element = Trackedelements[i];

                ZIndex[i] = Element->ZIndex;
                Tickcallbacks[i] = Element->onTick;
                Eventcallbacks[i] = Element->onEvent;
                Paintcallbacks[i] = Element->onPaint;
                Eventmasks[i] = Element->Eventmask.Raw;
                Hitbox[i] = { Element->Position.x, Element->Position.y,
                              Element->Position.x + Element->Size.x, Element->Position.y + Element->Size.y };
            }

            // Could probably create some better algorithm here..
            while (true)
            {
                // Find the first unordered element.
                const auto Element = std::ranges::is_sorted_until(ZIndex);
                if (Element == ZIndex.end()) break;

                // Find where the element belongs and the offsets.
                const auto Position = std::ranges::find_if(ZIndex, [=](const auto &Item) { return Item > *Element; });
                const auto A = std::ranges::distance(ZIndex.begin(), Position);
                const auto B = std::ranges::distance(ZIndex.begin(), Element);
                const auto Start = std::min(A, B);
                const auto End = std::max(A, B);

                // And rotate the whole cache around those points.
                #define Rotate(x) std::rotate(x .begin() + Start, x .begin() + End, x .begin() + End)
                Rotate(Trackedelements);
                Rotate(Paintcallbacks);
                Rotate(Eventcallbacks);
                Rotate(Tickcallbacks);
                Rotate(Eventmasks);
                Rotate(Hitbox);
                Rotate(ZIndex);
                #undef Rotate
            };

            // Probably needs a redraw.
            Invalidatescreen({}, Windowsize);
        }

        // Process the window events.
        MSG Message{};
        while (PeekMessageW(&Message, Windowhandle, NULL, NULL, PM_REMOVE))
        {
            TranslateMessage(&Message);
            DispatchMessageW(&Message);
        }

        // Tick the world forward.
        onTick(DeltatimeMS);

        // If a repaint is needed, notify the elements.
        if (dirtyBuffer.test()) [[unlikely]] onPaint();
    }

    // Only the base Overlay_t class handles the window events, for simplicity.
    static LRESULT __stdcall Overlaywndproc(HWND Windowhandle, UINT Message, WPARAM wParam, LPARAM lParam)
    {
        static bool modShift{}, modCtrl{};
        static HWND Mousecaptureowner{};
        Overlay_t *This{};

        // Need to save the class pointer on creation.
        if (Message == WM_NCCREATE) [[unlikely]]
        {
            This = (Overlay_t *)((LPCREATESTRUCTA)lParam)->lpCreateParams;
            SetWindowLongPtrA(Windowhandle, GWLP_USERDATA, (LONG_PTR)This);

            // Set up mouse-tracking.
            SetCapture(Windowhandle);
        }
        else
        {
            This = (Overlay_t *)GetWindowLongPtrA(Windowhandle, GWLP_USERDATA);
        }

        // May need to extend this in the future.
        switch (Message)
        {
            // Preferred over WM_MOVE/WM_SIZE for performance.
            case WM_WINDOWPOSCHANGED:
            {
                const auto Info = (WINDOWPOS *)lParam;
                This->Windowsize = { Info->cx, Info->cy };
                This->Windowposition = { Info->x, Info->y };
                This->Invalidatescreen({}, { Info->cx, Info->cy });

                Eventflags_t Flags{}; Flags.onWindowchange = true;
                This->onEvent(Flags, vec2i{ Info->cx, Info->cy });
                return NULL;
            }

            // Keyboard input.
            case WM_UNICHAR:
            case WM_CHAR:
            {
                // Why would you send this as a char?!
                if (VK_BACK == wParam) return NULL;

                Eventflags_t Flags{}; Flags.modDown = !(lParam & ( 1U << 31)); Flags.onCharinput = true;
                This->onEvent(Flags, uint32_t(wParam));
                return NULL;
            }

            // Special keys that need extra translating.
            case WM_KEYDOWN:
            case WM_KEYUP:
            {
                const auto isDown = !(lParam & (1 << 31));

                // Translate macros to extended codes.
                switch (wParam)
                {
                    case 'Z': if (modCtrl) wParam = modShift ? VK_REDO : VK_UNDO; break;
                    case 'V': if (modCtrl) wParam = VK_PASTE; break;
                    case 'C': if (modCtrl) wParam = VK_COPY; break;
                    case 'X': if (modCtrl) wParam = VK_CUT; break;

                    // Only update the state, no need to notify elements.
                    case VK_CONTROL: modCtrl = isDown; return NULL;
                    case VK_SHIFT: modShift = isDown; return NULL;
                }

                Eventflags_t Flags{};
                Flags.modDown = !(lParam & ( 1U << 31));
                Flags.modShift = modShift;
                Flags.modCtrl = modCtrl;
                Flags.onKey = true;

                This->onEvent(Flags, uint32_t(wParam));
                return NULL;
            }

            // Mouse capture.
            case WM_MOUSELEAVE:
            {
                Eventflags_t Flags{}; Flags.onMousemove = true;
                This->onEvent(Flags, vec2i{ -99999, -99999 }); // Probably outside of the positioning.
                Mousecaptureowner = {};
                return NULL;
            }
            case WM_MOUSEMOVE:
            {
                // Notify when the mouse leaves our window.
                if (Mousecaptureowner != Windowhandle)
                {
                    TRACKMOUSEEVENT Event{ sizeof(TRACKMOUSEEVENT), TME_LEAVE, Windowhandle };
                    Mousecaptureowner = Windowhandle;
                    TrackMouseEvent(&Event);
                }

                // Ensure that globals are set.
                modCtrl = wParam & MK_CONTROL;
                modShift = wParam & MK_SHIFT;

                Eventflags_t Flags{};
                Flags.modCtrl = modCtrl;
                Flags.onMousemove = true;
                Flags.modShift = modShift;
                This->onEvent(Flags, vec2i{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) });
                return NULL;
            }

            // Mouse input.
            case WM_LBUTTONDOWN:
            case WM_RBUTTONDOWN:
            case WM_MBUTTONDOWN:
            {
                // Notify when the mouse leaves our window.
                if (Mousecaptureowner != Windowhandle)
                {
                    TRACKMOUSEEVENT Event{ sizeof(TRACKMOUSEEVENT), TME_LEAVE, Windowhandle };
                    Mousecaptureowner = Windowhandle;
                    TrackMouseEvent(&Event);
                }

                // Ensure that globals are set.
                modCtrl = wParam & MK_CONTROL;
                modShift = wParam & MK_SHIFT;

                // Translate to normal codes.
                do
                {
                    if (wParam & MK_LBUTTON) { wParam = VK_LBUTTON; break; }
                    if (wParam & MK_RBUTTON) { wParam = VK_RBUTTON; break; }
                    if (wParam & MK_MBUTTON) [[unlikely]] { wParam = VK_MBUTTON; break; }
                    if (wParam & MK_XBUTTON1) [[unlikely]] { wParam = VK_XBUTTON1; break; }
                    if (wParam & MK_XBUTTON2) [[unlikely]] { wParam = VK_XBUTTON2; break; }
                } while (false);

                // Bit of a hack, send a move event first.
                {
                    Eventflags_t Flags{};
                    Flags.modCtrl = modCtrl;
                    Flags.onMousemove = true;
                    Flags.modShift = modShift;
                    This->onEvent(Flags, vec2i{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) });
                }
                {
                    Eventflags_t Flags{};
                    Flags.onClick = true;
                    Flags.modDown = true;
                    Flags.modCtrl = modCtrl;
                    Flags.modShift = modShift;
                    This->onEvent(Flags, uint32_t(wParam));
                }

                return NULL;
            }
            case WM_LBUTTONUP:
            case WM_RBUTTONUP:
            case WM_MBUTTONUP:
            {
                // Notify when the mouse leaves our window.
                if (Mousecaptureowner != Windowhandle)
                {
                    TRACKMOUSEEVENT Event{ sizeof(TRACKMOUSEEVENT), TME_LEAVE, Windowhandle };
                    Mousecaptureowner = Windowhandle;
                    TrackMouseEvent(&Event);
                }

                // Ensure that globals are set.
                modCtrl = wParam & MK_CONTROL;
                modShift = wParam & MK_SHIFT;

                // Translate to normal codes.
                do
                {
                    if (wParam & MK_LBUTTON) { wParam = VK_LBUTTON; break; }
                    if (wParam & MK_RBUTTON) { wParam = VK_RBUTTON; break; }
                    if (wParam & MK_MBUTTON) [[unlikely]] { wParam = VK_MBUTTON; break; }
                    if (wParam & MK_XBUTTON1) [[unlikely]] { wParam = VK_XBUTTON1; break; }
                    if (wParam & MK_XBUTTON2) [[unlikely]] { wParam = VK_XBUTTON2; break; }
                } while (false);

                // Bit of a hack, send a move event first.
                {
                    Eventflags_t Flags{};
                    Flags.modCtrl = modCtrl;
                    Flags.onMousemove = true;
                    Flags.modShift = modShift;
                    This->onEvent(Flags, vec2i{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) });
                }
                {
                    Eventflags_t Flags{};
                    Flags.onClick = true;
                    Flags.modDown = false;
                    Flags.modCtrl = modCtrl;
                    Flags.modShift = modShift;
                    This->onEvent(Flags, uint32_t(wParam));
                }

                return NULL;
            }

            // Generally I let the system clean up, but incase this is used in the future.
            case WM_DESTROY:
            {
                ReleaseCapture();

                if (Mousecaptureowner == Windowhandle)
                {
                    TRACKMOUSEEVENT Event{ sizeof(TRACKMOUSEEVENT), TME_CANCEL, Windowhandle };
                    TrackMouseEvent(&Event);
                }
            }
        }

        return DefWindowProcW(Windowhandle, Message, wParam, lParam);
    }

    // Create a new window for our overlay.
    Overlay_t(vec2i Position, vec2i Size) : Windowposition(Position), Windowsize(Size)
    {
        // Register the overlay class.
        WNDCLASSEXW Windowclass{};
        Windowclass.style = CS_DBLCLKS;
        Windowclass.cbSize = sizeof(WNDCLASSEXW);
        Windowclass.lpfnWndProc = Overlaywndproc;
        Windowclass.lpszClassName = L"Ayria_overlay";
        Windowclass.cbWndExtra = sizeof(Overlay_t *);
        if (NULL == RegisterClassExW(&Windowclass)) assert(false);

        // Generic overlay style.
        const DWORD Style = WS_POPUP | (Build::isDebug * WS_BORDER);
        const DWORD StyleEx = WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED;

        // Topmost, transparent, no icon on the taskbar, zero size so it's not shown.
        Windowhandle = CreateWindowExW(StyleEx, Windowclass.lpszClassName, NULL, Style, 0, 0, 0, 0, NULL, NULL, NULL, this);
        assert(Windowhandle);

        // Use a pixel-value to mean transparent rather than Alpha, because using Alpha is slow.
        SetLayeredWindowAttributes(Windowhandle, 0x00FFFFFF, 0, LWA_COLORKEY);

        // Resize to show the window.
        if (Position) SetWindowPos(Windowhandle, NULL, Position.x, Position.y, Size.x, Size.y, SWP_ASYNCWINDOWPOS | SWP_NOSIZE);
        if (Size) SetWindowPos(Windowhandle, NULL, Position.x, Position.y, Size.x, Size.y, SWP_ASYNCWINDOWPOS | SWP_NOMOVE);
    }
};

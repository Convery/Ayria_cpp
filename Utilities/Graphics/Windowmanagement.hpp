/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-07-20
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>

#include "Renderer.hpp"
#include "../Datatypes.hpp"
#include "Utilities/Threads/Spinlock.hpp"

// Windows only module.
static_assert(Build::isWindows, "Window management is only available on Windows for now.");

constexpr bool dbgUpdate = false;

namespace Graphics
{
    // VK 0x3A-0x40 are undefined on Windows, let's reuse them.
    enum VK_EXTENDED
    {
        VK_CUT = 0x3A,
        VK_COPY,
        VK_PASTE,
        VK_UNDO,
        VK_REDO,
    };

    // For easier signaling.
    union Eventflags_t
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
    class Window_t;
    using Tickcallback = std::function<void(Window_t *Parent, uint32_t DeltatimeMS)>;
    using Paintcallback = std::function<void(Window_t *Parent, const struct Renderer_t &Renderer)>;
    using Eventcallback = std::function<void(Window_t *Parent, Eventflags_t Eventtype, const std::variant<uint32_t, vec2i> &Eventdata)>;

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
    class Window_t
    {
        protected:
        HRGN dirtyRegion{ CreateRectRgn(0, 0, 0, 0) };
        std::atomic_flag dirtyElements{};
        Spinlock Threadlock{};

        // Inline element manager.
        struct
        {
            std::vector<vec4i> Hitboxes{};
            std::vector<Eventflags_t> Eventmasks{};
            std::vector<Tickcallback> Tickcallbacks{};
            std::vector<Eventcallback> Eventcallbacks{};
            std::vector<Paintcallback> Paintcallbacks{};
            std::vector<Elementinfo_t *> Trackedelements{};

            void Removeelement(Elementinfo_t *Element)
            {
                const auto Result = std::ranges::find(Trackedelements, Element);
                if (Result != Trackedelements.end()) Trackedelements.erase(Result);
            }
            void Trackelement(Elementinfo_t *Element)
            {
                Trackedelements.emplace_back(Element);
            }
            void Updatefocus(vec2i Point)
            {
                const auto Range = lz::zip(Trackedelements, Hitboxes);
                std::for_each(std::execution::par_unseq, Range.begin(), Range.end(), [&](const auto &Tuple)
                {
                    const auto &[Element, Box] = Tuple;

                    if (Point.x >= Box.x && Point.x <= Box.z && Point.y >= Box.y && Point.y <= Box.w)
                    {
                        Element->isFocused.test_and_set();
                    }
                    else
                    {
                        Element->isFocused.clear();
                    }
                });
            }
            void Reinitialize()
            {
                Hitboxes.resize(Trackedelements.size());
                Eventmasks.resize(Trackedelements.size());
                Tickcallbacks.resize(Trackedelements.size());
                Paintcallbacks.resize(Trackedelements.size());
                Eventcallbacks.resize(Trackedelements.size());

                std::ranges::sort(Trackedelements, {}, &Elementinfo_t::ZIndex);
                for (const auto &[Index, Element] : lz::enumerate(Trackedelements))
                {
                    Eventmasks[Index] = Element->Eventmask;
                    Tickcallbacks[Index] = Element->onTick;
                    Paintcallbacks[Index] = Element->onPaint;
                    Eventcallbacks[Index] = Element->onEvent;

                    Hitboxes[Index] = { Element->Position.x, Element->Position.y,
                        Element->Position.x + Element->Size.x, Element->Position.y + Element->Size.y };
                }
            }
        } Elements{};

        // Internal helpers.
        static inline bool needsPaint(HRGN Dirty)
        {
            static auto Clean = CreateRectRgn(0, 0, 0, 0);
            return !EqualRgn(Dirty, Clean);
        }

        // Callbacks that could be extended.
        virtual void onPaint()
        {
            RECT Dirtyrect{};
            if (1 >= GetRgnBox(dirtyRegion, &Dirtyrect)) [[unlikely]] return;

            const vec2u Size(Dirtyrect.right - Dirtyrect.left, Dirtyrect.bottom - Dirtyrect.top);
            const vec4i Paintrect(Dirtyrect.left, Dirtyrect.top, Dirtyrect.right, Dirtyrect.bottom);

            const auto Windowcontext = GetDCEx(Windowhandle, NULL, DCX_LOCKWINDOWUPDATE | DCX_VALIDATE);
            const auto Devicecontext = CreateCompatibleDC(Windowcontext);
            const auto BMP = CreateCompatibleBitmap(Windowcontext, Size.x, Size.y);
            const auto Old = SelectObject(Devicecontext, BMP);

            // Clean the new bitmap.
            const Graphics::Renderer_t Renderer(Devicecontext);
            BitBlt(Devicecontext, 0, 0, Size.x, Size.y, NULL, 0, 0, WHITENESS);

            // Update the viewport so that the elemements can use natural coordinates.
            SetViewportOrgEx(Devicecontext, 0 - Dirtyrect.left, 0 - Dirtyrect.top, NULL);

            // Drawing needs to be sequential, skip elements that are out of view.
            const auto Range = lz::zip(Elements.Paintcallbacks, Elements.Hitboxes);
            for (const auto &[Callback, Box] : Range)
            {
                if (!Callback) [[unlikely]] continue;
                if (Box.x > Paintrect.z || Box.z < Paintrect.x) continue;
                if (Box.y > Paintrect.w || Box.w < Paintrect.y) continue;

                Callback(this, Renderer);
            }

            // For debugging, draw an outline over the elements.
            if constexpr (Build::isDebug && dbgUpdate)
            {
                static Color_t Rainbow[6] = { Color_t(168, 0, 255), Color_t(0, 121, 255), Color_t(0, 241, 29),
                                              Color_t(255, 239, 0), Color_t(255, 127, 0), Color_t(255, 9, 0) };

                const auto dbgRange = lz::zip(Elements.Trackedelements, Elements.Hitboxes);
                for (const auto &[Element, Box] : dbgRange)
                {
                    if (!Element->onPaint) [[unlikely]] continue;
                    if (Box.x > Paintrect.z || Box.z < Paintrect.x) continue;
                    if (Box.y > Paintrect.w || Box.w < Paintrect.y) continue;

                    Renderer.Rectangle({ Box.x, Box.y }, { Box.z - Box.x, Box.w - Box.y }, 0, 2)->Render(Rainbow[Element->ZIndex % 6], {});
                }
            }

            // And blit to the screen.
            BitBlt(Windowcontext, Paintrect.x, Paintrect.y, Size.x, Size.y, Devicecontext, Dirtyrect.left, Dirtyrect.top, SRCCOPY);

            DeleteObject(SelectObject(Devicecontext, Old));
            DeleteDC(Devicecontext);
            ReleaseDC(Windowhandle, Windowcontext);

            // Clear the region.
            ValidateRgn(Windowhandle, dirtyRegion);
            CombineRgn(dirtyRegion, dirtyRegion, dirtyRegion, RGN_XOR);
        }
        virtual void onTick(uint32_t DeltatimeMS)
        {
            std::for_each(std::execution::par_unseq, Elements.Tickcallbacks.cbegin(), Elements.Tickcallbacks.cend(),
                          [&](auto &Callback) { if (Callback) [[likely]] Callback(this, DeltatimeMS); });
        }
        virtual void onEvent(Eventflags_t Eventtype, std::variant<uint32_t, vec2i> Eventdata)
        {
            // Basic hit detection.
            if (Eventtype.onMousemove)
            {
                Elements.Updatefocus(std::get<vec2i>(Eventdata));
            }

            // We do not care about the order of elements here.
            const auto Range = lz::zip(Elements.Eventcallbacks, Elements.Eventmasks);
            std::for_each(std::execution::par_unseq, Range.begin(), Range.end(), [&](const auto &Tuple)
            {
                const auto &[Callback, Mask] = Tuple;

                if ((Eventtype.Raw & Mask.Raw) && Callback) [[likely]]
                {
                    Callback(this, Eventtype, Eventdata);
                }
            });
        }

        public:
        bool isVisible{};
        vec2u Windowsize{};
        HWND Windowhandle{};
        vec2i Windowposition{};

        // Access from the elements.
        void Togglevisibility()
        {
            setVisibility(isVisible ^ 1);
        }
        void setVisibility(bool Visible)
        {
            isVisible = Visible;
            ShowWindowAsync(Windowhandle, isVisible ? SW_SHOW : SW_HIDE);
            if (Visible)
            {
                Invalidatescreen({}, Windowsize);
                SetActiveWindow(Windowhandle);
            }
        }
        void Insertelement(Elementinfo_t *Element)
        {
            assert(Element);
            std::scoped_lock Guard(Threadlock);

            Elements.Trackelement(Element);
            dirtyElements.test_and_set();
        }
        void Removelement(Elementinfo_t *Element)
        {
            assert(Element);
            std::scoped_lock Guard(Threadlock);

            Elements.Removeelement(Element);
            dirtyElements.test_and_set();
        }
        void Resize(vec2i Position, vec2u Size) const
        {
            if (Position && Size) SetWindowPos(Windowhandle, NULL, Position.x, Position.y, Size.x, Size.y, SWP_ASYNCWINDOWPOS);
            else if (Size) SetWindowPos(Windowhandle, NULL, Position.x, Position.y, Size.x, Size.y, SWP_ASYNCWINDOWPOS | SWP_NOMOVE);
            else if (Position) SetWindowPos(Windowhandle, NULL, Position.x, Position.y, Size.x, Size.y, SWP_ASYNCWINDOWPOS | SWP_NOSIZE);

            Invalidatescreen({}, Size);
            SetActiveWindow(Windowhandle);
        }
        void Invalidatescreen(vec4u &&Dirtyrect) const
        {
            const auto Addition = CreateRectRgn(Dirtyrect.x, Dirtyrect.y, Dirtyrect.z, Dirtyrect.w);
            CombineRgn(dirtyRegion, dirtyRegion, Addition, RGN_OR);
            DeleteObject(Addition);
        }
        void Invalidatescreen(vec2i Dirtypos, const vec2u &Dirtysize) const
        {
            return Invalidatescreen(vec4i{ Dirtypos.x, Dirtypos.y, Dirtypos.x + Dirtysize.x, Dirtypos.y + Dirtysize.y });
        }

        // Called each frame, dispatches to derived classes.
        void onFrame(uint32_t DeltatimeMS)
        {
            // If the element-cache is dirty, we need to sort it again.
            if (dirtyElements.test()) [[unlikely]]
            {
                std::scoped_lock Guard(Threadlock);
                Elements.Reinitialize();
                dirtyElements.clear();

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
            if (needsPaint(dirtyRegion)) [[unlikely]] onPaint();
        }

        // Only the base Window_t class handles the window events, for simplicity.
        static LRESULT __stdcall Overlaywndproc(HWND Windowhandle, UINT Message, WPARAM wParam, LPARAM lParam)
        {
            static bool modShift{}, modCtrl{};
            static HWND Mousecaptureowner{};
            Window_t *This{};

            // Need to save the class pointer on creation.
            if (Message == WM_NCCREATE) [[unlikely]]
            {
                This = static_cast<Window_t *>(reinterpret_cast<LPCREATESTRUCTA>(lParam)->lpCreateParams);
                SetWindowLongPtrA(Windowhandle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(This));

                // Set up mouse-tracking.
                SetCapture(Windowhandle);
            }
            else
            {
                This = reinterpret_cast<Window_t *>(GetWindowLongPtrA(Windowhandle, GWLP_USERDATA));
            }

            // May need to extend this in the future.
            switch (Message)
            {
                // While we track most invalidation internally, just in case..
                case WM_PAINT:
                {
                    RECT Updaterect;
                    if (GetUpdateRect(This->Windowhandle, &Updaterect, FALSE))
                    {
                        This->Invalidatescreen({ Updaterect.left, Updaterect.top, Updaterect.right, Updaterect.bottom });
                    }

                    ValidateRect(This->Windowhandle, NULL);
                    return NULL;
                }

                // Preferred over WM_MOVE/WM_SIZE for performance.
                case WM_WINDOWPOSCHANGED:
                {
                    const auto Info = (WINDOWPOS *)lParam;
                    This->Windowsize = { Info->cx, Info->cy };
                    This->Windowposition = { Info->x, Info->y };
                    This->Invalidatescreen({}, { Info->cx, Info->cy });

                    Eventflags_t Flags{}; Flags.onWindowchange = true;
                    This->onEvent(Flags, vec2i{ Info->cx, Info->cy });

                    // Elements have probably been resized.
                    This->dirtyElements.test_and_set();
                    return NULL;
                }

                // Keyboard input.
                case WM_UNICHAR:
                case WM_CHAR:
                {
                    // Why would you send this as a char?!
                    if (VK_BACK == wParam) return NULL;

                    Eventflags_t Flags{}; Flags.modDown = !(lParam & (1U << 31)); Flags.onCharinput = true;
                    This->onEvent(Flags, static_cast<uint32_t>(wParam));
                    return NULL;
                }

                // Special keys that need extra translating.
                case WM_KEYDOWN:
                case WM_KEYUP:
                {
                    const auto isDown = !(lParam & (1U << 31));

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
                    Flags.modDown = !(lParam & (1U << 31));
                    Flags.modShift = modShift;
                    Flags.modCtrl = modCtrl;
                    Flags.onKey = true;

                    This->onEvent(Flags, static_cast<uint32_t>(wParam));
                    return NULL;
                }

                // Mouse capture.
                case WM_MOUSELEAVE:
                {
                    Eventflags_t Flags{}; Flags.onMousemove = true;
                    This->onEvent(Flags, vec2i{ INT16_MIN, INT16_MIN });
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
                        This->onEvent(Flags, static_cast<uint32_t>(wParam));
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
                        This->onEvent(Flags, static_cast<uint32_t>(wParam));
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
                default: ;
            }

            return DefWindowProcW(Windowhandle, Message, wParam, lParam);
        }

        // Create a new window for our overlay.
        Window_t() = default;
        Window_t(vec2i Position, vec2u Size) : Windowsize(Size), Windowposition(Position)
        {
            // Register the overlay class.
            WNDCLASSEXW Windowclass{};
            Windowclass.cbSize = sizeof(WNDCLASSEXW);
            Windowclass.lpfnWndProc = Overlaywndproc;
            Windowclass.lpszClassName = L"Ayria_window";
            Windowclass.cbWndExtra = sizeof(Window_t *);
            Windowclass.style = CS_DBLCLKS | CS_SAVEBITS | CS_CLASSDC | CS_BYTEALIGNCLIENT | CS_BYTEALIGNWINDOW;
            RegisterClassExW(&Windowclass);

            // Generic overlay style.
            constexpr DWORD Style = WS_POPUP;
            constexpr DWORD StyleEx = WS_EX_LAYERED;

            // Topmost, transparent, no icon on the taskbar, zero size so it's not shown.
            Windowhandle = CreateWindowExW(StyleEx, Windowclass.lpszClassName, NULL, Style, 0, 0, 0, 0, NULL, NULL, NULL, this);
            assert(Windowhandle);

            // Use a pixel-value to mean transparent rather than Alpha, because using Alpha is slow.
            SetLayeredWindowAttributes(Windowhandle, 0x00FFFFFF, 0, LWA_COLORKEY);

            // Resize to show the window.
            if (Position) SetWindowPos(Windowhandle, NULL, Position.x, Position.y, Size.x, Size.y, SWP_ASYNCWINDOWPOS | SWP_NOSIZE);
            if (Size) SetWindowPos(Windowhandle, NULL, Position.x, Position.y, Size.x, Size.y, SWP_ASYNCWINDOWPOS | SWP_NOMOVE);
        }

        // In-case someone does something silly.
        virtual ~Window_t() = default;
    };

    // Top-most overlay to track another window.
    class Overlay_t : public Window_t
    {
        // Create a new window for our overlay.
        Overlay_t(vec2i Position, vec2u Size)
        {
            Windowsize = Size;
            Windowposition = Position;

            // Register the overlay class.
            WNDCLASSEXW Windowclass{};
            Windowclass.cbSize = sizeof(WNDCLASSEXW);
            Windowclass.lpfnWndProc = Overlaywndproc;
            Windowclass.lpszClassName = L"Ayria_overlay";
            Windowclass.cbWndExtra = sizeof(Overlay_t *);
            Windowclass.style = CS_DBLCLKS | CS_SAVEBITS | CS_CLASSDC | CS_BYTEALIGNCLIENT | CS_BYTEALIGNWINDOW;
            RegisterClassExW(&Windowclass);

            // Generic overlay style.
            constexpr DWORD Style = WS_POPUP;
            constexpr DWORD StyleEx = WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED;

            // Topmost, transparent, no icon on the taskbar, zero size so it's not shown.
            Windowhandle = CreateWindowExW(StyleEx, Windowclass.lpszClassName, NULL, Style, 0, 0, 0, 0, NULL, NULL, NULL, this);
            assert(Windowhandle);

            // Use a pixel-value to mean transparent rather than Alpha, because using Alpha is slow.
            SetLayeredWindowAttributes(Windowhandle, 0x00FFFFFF, 0, LWA_COLORKEY);

            // Resize to show the window.
            if (Position) SetWindowPos(Windowhandle, NULL, Position.x, Position.y, Size.x, Size.y, SWP_ASYNCWINDOWPOS | SWP_NOSIZE);
            if (Size) SetWindowPos(Windowhandle, NULL, Position.x, Position.y, Size.x, Size.y, SWP_ASYNCWINDOWPOS | SWP_NOMOVE);
        }

        // In-case someone does something silly.
        virtual ~Overlay_t() = default;
    };
}

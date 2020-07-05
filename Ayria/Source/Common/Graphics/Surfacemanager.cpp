/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-07-04
    License: MIT
*/

#include <Stdinclude.hpp>

namespace Graphics
{
    struct Event_t
    {
        union
        {
            uint32_t Raw;
            uint32_t
                Mousemove : 1,
                Mousedown : 1,
                Mouseup : 1,
                Keydown : 1,
                Keyup : 1,
                doCut : 1,
                doCopy : 1,
                doPaste : 1,
                doCancel : 1,
                doUndo : 1,
                doRedo : 1,
                doBackspace : 1,
                doDelete : 1,
                doEnter : 1,
                doTab : 1,
                onCharinput : 1,
                onWindowchange : 1;
        } Flags;

        union
        {
            POINT Point;
            uint32_t Keycode;
            char Letter;
        } Data;
    };

    struct Surface_t;
    struct Element_t
    {
        uint32_t Wantedevents;
        POINT Position, Size;
        HDC Devicecontext;
        HBITMAP Mask;

        // Update the internal state.
        std::function<void (Surface_t *, Element_t *)> onRender;
        std::function<void (Surface_t *, Element_t *, float)> onFrame;
        std::function<void (Surface_t *, Element_t *, Event_t)> onEvent;
    };
    struct Surface_t
    {
        HDC Devicecontext;
        HWND Windowhandle;
        POINT Mouseposition;
        POINT Position, Size;
        bool isVisible, isDirty;
        std::vector<Element_t> Elements;
    };

    constexpr COLORREF toColor(uint8_t R, uint8_t G, uint8_t B, uint32_t A)
    {
        return R | (G << 8) | (B << 16) | (A << 24);
    }

    std::unordered_map<std::string, Surface_t> Surfaces;
    constexpr auto Clearcolor = toColor(0xFF, 0xFF, 0xFF, 0xFF);
    const HBRUSH Clearbrush = CreateSolidBrush(Clearcolor);

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
                            Surface.isDirty = true;
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
                            Surface.isDirty = true;

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
                            if (Item.onEvent)
                            {
                                Item.onEvent(&Surface, &Item, Event);
                            }
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
                if(Element.onFrame)
                {
                    Element.onFrame(&Surface, &Element, Deltatime);
                }
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
            for(auto &Element : Surface.Elements)
            {
                if (Element.onRender)
                {
                    Element.onRender(&Surface, &Element);
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
            }

            // Paint the screen.
            BitBlt(GetDC(Surface.Windowhandle), 0, 0, Surface.Size.x, Surface.Size.y, Surface.Devicecontext, 0, 0, SRCCOPY);

            // Ensure that the surface is visible.
            ShowWindowAsync(Surface.Windowhandle, SW_SHOWNORMAL);
        }
    }

    namespace Paint
    {
        struct Pen_t
        {
            static std::unordered_map<int, HPEN> Pens;
            HGDIOBJ Previous;
            HDC DC;

            explicit Pen_t(HDC Devicecontext, int Linewidth, COLORREF Colour)
            {
                auto &Pen = Pens[Linewidth];
                if (Pen->unused == NULL) [[unlikely]] Pen = CreatePen(PS_SOLID, Linewidth, Colour);
                Previous = SelectObject(Devicecontext, Pen);
                SetDCPenColor(Devicecontext, Colour);
                DC = Devicecontext;
            }
            ~Pen_t()
            {
                SelectObject(DC, Previous);
            }
        };

        inline void setClipping(HDC Devicecontext, RECT Area)
        {
            SelectClipRgn(Devicecontext, NULL);
            IntersectClipRect(Devicecontext, Area.left, Area.top, Area.right, Area.bottom);
        }
        inline HBITMAP Createmask(HDC Devicecontext, POINT Size, COLORREF Transparancykey)
        {
            const auto Bitmap = CreateBitmap(Size.x, Size.y, 1, 1, NULL);
            const auto Device = CreateCompatibleDC(Devicecontext);

            SetBkColor(Devicecontext, Transparancykey);
            SelectObject(Device, Bitmap);

            BitBlt(Device, 0, 0, Size.x, Size.y, Devicecontext, 0, 0, SRCCOPY);
            BitBlt(Device, 0, 0, Size.x, Size.y, Device, 0, 0, SRCINVERT);

            DeleteDC(Device);
            return Bitmap;
        }
        inline void Image(HDC Devicecontext, POINT Position, POINT Size, std::pair<BITMAP, HBITMAP> Bitmap)
        {
            const auto Device = CreateCompatibleDC(Devicecontext);
            SelectObject(Device, Bitmap.second);

            if (Size.x >= Bitmap.first.bmWidth && Size.y >= std::abs(Bitmap.first.bmHeight))
            {
                BitBlt(Devicecontext, Position.x, Position.y, Size.x, Size.y, Device, 0, 0, SRCCOPY);
            }
            else
            {
                StretchBlt(Devicecontext, Position.x, Position.y, Size.x, Size.y, Device, 0, 0, Bitmap.first.bmWidth, Bitmap.first.bmHeight, SRCCOPY);
            }
            DeleteDC(Device);
        }

        #define COMMON HDC Devicecontext, POINT Position, COLORREF Colour
        namespace Lines
        {
            inline void Singleline(COMMON, int Linewidth, POINT Endpoint)
            {
                const Pen_t Pen(Devicecontext, Linewidth, Colour);

                MoveToEx(Devicecontext, Position.x, Position.y, NULL);
                LineTo(Devicecontext, Endpoint.x, Endpoint.y);
            }
            inline void Polyline(COMMON, int Linewidth, POINT *Points, int Pointcount)
            {
                const Pen_t Pen(Devicecontext, Linewidth, Colour);
                ::Polyline(Devicecontext, Points, Pointcount);
            }
        }

        namespace Curves
        {
            inline void Arc(COMMON, POINTFLOAT Angles, int Rounding, int Linewidth)
            {
                const Pen_t Pen(Devicecontext, Linewidth, Colour);

                const float X0 = Position.x + Rounding * std::cosf((Angles.x + Angles.y) * std::numbers::pi / 180.0f);
                const float Y0 = Position.y + Rounding * std::sinf((Angles.x + Angles.y) * std::numbers::pi / 180.0f);
                const float X1 = Position.x + Rounding * std::cosf(Angles.x * std::numbers::pi / 180.0f);
                const float Y1 = Position.y + Rounding * std::sinf(Angles.x * std::numbers::pi / 180.0f);

                SetArcDirection(Devicecontext, AD_COUNTERCLOCKWISE);
                Arc(Devicecontext, Position.x - Rounding, Position.y - Rounding,
                    Position.x + Rounding, Position.y + Rounding,
                    (int)X0, (int)Y0, (int)X1, (int)Y1);
            }
            inline void Pie(COMMON, POINTFLOAT Angles, int Rounding, int Linewidth, COLORREF Background)
            {
                const Pen_t Pen(Devicecontext, Linewidth, Colour);
                SetDCBrushColor(Devicecontext, Background);

                const float X0 = Position.x + Rounding * std::cosf((Angles.x + Angles.y) * std::numbers::pi / 180.0f);
                const float Y0 = Position.y + Rounding * std::sinf((Angles.x + Angles.y) * std::numbers::pi / 180.0f);
                const float X1 = Position.x + Rounding * std::cosf(Angles.x * std::numbers::pi / 180.0f);
                const float Y1 = Position.y + Rounding * std::sinf(Angles.x * std::numbers::pi / 180.0f);

                SetArcDirection(Devicecontext, AD_COUNTERCLOCKWISE);
                ::Pie(Devicecontext, Position.x - Rounding, Position.y - Rounding,
                    Position.x + Rounding, Position.y + Rounding,
                    (int)X0, (int)Y0, (int)X1, (int)Y1);
            }
        }

        namespace Ellipse
        {
            inline void Outline(COMMON, POINT Size, int Linewidth)
            {
                const Pen_t Pen(Devicecontext, Linewidth, Colour);
                SetDCBrushColor(Devicecontext, OPAQUE);
                ::Ellipse(Devicecontext, Position.x, Position.y, Position.x + Size.x, Position.y + Size.y);
            }
            inline void Solid(COMMON, POINT Size)
            {
                SetDCPenColor(Devicecontext, Colour);
                SetDCBrushColor(Devicecontext, Colour);
                ::Ellipse(Devicecontext, Position.x, Position.y, Position.x + Size.x, Position.y + Size.y);
            }
            inline void Filled(COMMON, POINT Size, int Linewidth, COLORREF Fillcolour)
            {
                const Pen_t Pen(Devicecontext, Linewidth, Colour);
                SetDCBrushColor(Devicecontext, Fillcolour);
                ::Ellipse(Devicecontext, Position.x, Position.y, Position.x + Size.x, Position.y + Size.y);
            }
        }

        namespace Rectangle
        {
            inline void Outline(COMMON, POINT Size, int Rounding, int Linewidth)
            {
                const Pen_t Pen(Devicecontext, Linewidth, Colour);
                SetDCBrushColor(Devicecontext, OPAQUE);

                if (Rounding == 0)
                {
                    ::Rectangle(Devicecontext, Position.x, Position.y, Position.x + Size.x, Position.y + Size.y);
                }
                else
                {
                    ::RoundRect(Devicecontext, Position.x, Position.y, Position.x + Size.x, Position.y + Size.y, Rounding, Rounding);
                }
            }
            inline void Solid(COMMON, POINT Size, int Rounding)
            {
                if (Rounding == 0)
                {
                    // NOTE(tcn): Looks odd but seems faster than Rectangle() for solid colours.
                    const RECT Area = { Position.x, Position.y, Position.x + Size.x, Position.y + Size.y };
                    SetBkColor(Devicecontext, Colour);
                    ExtTextOutW(Devicecontext, 0, 0, ETO_OPAQUE, &Area, NULL, 0, NULL);
                }
                else
                {
                    SetDCPenColor(Devicecontext, Colour);
                    SetDCBrushColor(Devicecontext, Colour);
                    ::RoundRect(Devicecontext, Position.x, Position.y, Position.x + Size.x, Position.y + Size.y, Rounding, Rounding);
                }
            }
            inline void Filled(COMMON, POINT Size, int Rounding, int Linewidth, COLORREF Fillcolour)
            {
                const Pen_t Pen(Devicecontext, Linewidth, Colour);
                SetDCBrushColor(Devicecontext, Fillcolour);

                if (Rounding == 0)
                {
                    ::Rectangle(Devicecontext, Position.x, Position.y, Position.x + Size.x, Position.y + Size.y);
                }
                else
                {
                    ::RoundRect(Devicecontext, Position.x, Position.y, Position.x + Size.x, Position.y + Size.y, Rounding, Rounding);
                }
            }
        }

        namespace Polygon
        {
            inline void Outline(COMMON, int Linewidth, std::vector<POINT> Points)
            {
                const auto Array = (POINT *)alloca(Points.size() * sizeof(POINT));
                for (int i = 0; i < Points.size(); ++i)
                {
                    Array[i].x = Points[i].x;
                    Array[i].y = Points[i].y;
                }

                Lines::Polyline(Devicecontext, Position, Colour, Linewidth, Array, Points.size());
            }
            inline void Solid(COMMON, int Linewidth, std::vector<POINT> Points)
            {
                const auto Array = (POINT *)alloca(Points.size() * sizeof(POINT));
                const Pen_t Pen(Devicecontext, Linewidth, Colour);
                for (int i = 0; i < Points.size(); ++i)
                {
                    Array[i].x = Points[i].x;
                    Array[i].y = Points[i].y;
                }

                SetDCBrushColor(Devicecontext, Colour);
                ::Polygon(Devicecontext, Array, Points.size());
            }
            inline void Filled(COMMON, int Linewidth, std::vector<POINT> Points, COLORREF Fillcolour)
            {
                const auto Array = (POINT *)alloca(Points.size() * sizeof(POINT));
                const Pen_t Pen(Devicecontext, Linewidth, Colour);
                for (int i = 0; i < Points.size(); ++i)
                {
                    Array[i].x = Points[i].x;
                    Array[i].y = Points[i].y;
                }

                SetDCBrushColor(Devicecontext, Fillcolour);
                ::Polygon(Devicecontext, Array, Points.size());
            }
            inline void Mesh(HDC Devicecontext, std::vector<POINT> Points, std::vector<COLORREF> Colours)
            {
                assert(Points.size() == Colours.size());
                const auto toVertex = [](POINT P, COLORREF C) -> TRIVERTEX
                {
                    return { P.x, P.y, (C & 0xFF) << 8, (C & 0xFF00) << 8, (C & 0xFF0000) << 8, (C & 0xFF000000) << 8 };
                };
                const auto Array = (TRIVERTEX *)alloca(Points.size() * sizeof(TRIVERTEX));
                for (int i = 0; i < Points.size(); ++i) Array[i] = toVertex(Points[i], Colours[i]);

                // Malformed quad, draw as two triangles.
                if (Points.size() == 4)
                {
                    GRADIENT_TRIANGLE Index[2] = { {0, 1, 3}, {2, 1, 3} };
                    GradientFill(Devicecontext, Array, Points.size(), Index, 2, GRADIENT_FILL_TRIANGLE);
                }
                else
                {
                    // Assume that the points are in order.
                    const auto Index = (GRADIENT_TRIANGLE *)alloca(Points.size() / 3 * sizeof(GRADIENT_TRIANGLE));
                    for (int i = 0; i < Points.size() / 3; ++i) Index[i] = { i, i + 1, i + 2 };
                    GradientFill(Devicecontext, Array, Points.size(), Index, Points.size() / 3, GRADIENT_FILL_TRIANGLE);
                }
            }
        }

        namespace Text
        {
            inline void Transparant(COMMON, HFONT Fonthandle, std::string_view String)
            {
                const auto Wide = toWide(String);
                SetTextColor(Devicecontext, Colour);
                SelectObject(Devicecontext, Fonthandle);
                ExtTextOutW(Devicecontext, Position.x, Position.y, ETO_NUMERICSLATIN, NULL, Wide.c_str(), Wide.size(), NULL);
            }
            inline void Opaque(COMMON, HFONT Fonthandle, std::string_view String, COLORREF Background)
            {
                const auto Wide = toWide(String);
                SetTextColor(Devicecontext, Colour);
                SetBkColor(Devicecontext, Background);
                SelectObject(Devicecontext, Fonthandle);
                ExtTextOutW(Devicecontext, Position.x, Position.y, ETO_NUMERICSLATIN | ETO_OPAQUE, NULL, Wide.c_str(), Wide.size(), NULL);
            }
            inline void setAlignment(HDC Devicecontext, bool Left, bool Right, bool Centre)
            {
                uint32_t Current = GetTextAlign(Devicecontext);
                if (Left) Current = (Current & ~TA_RIGHT) | TA_LEFT;
                if (Right) Current = (Current & ~TA_LEFT) | TA_RIGHT;
                if (Centre) Current = (Current & ~(TA_LEFT | TA_RIGHT)) | TA_CENTER;

                SetTextAlign(Devicecontext, Current);
            }
        }

        #undef COMMON
    }

    namespace Fonts
    {
        inline POINT getTextsize(HDC Devicecontext, std::string_view Text)
        {
            SIZE Textsize;
            const auto String = toWide(Text);

            if (GetTextExtentPoint32W(Devicecontext, String.c_str(), int(String.size()), &Textsize))
                return { Textsize.cx, Textsize.cy };
            return {};
        }

        inline HFONT Createfont(const char *Name, int32_t Fontsize)
        {
            return CreateFontA(Fontsize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, Name);

        }
        inline HFONT Createfont(const char *Name, int32_t Fontsize, void *Fontdata, uint32_t Datasize)
        {
            DWORD Installedfonts{};
            (void)AddFontMemResourceEx(Fontdata, Datasize, NULL, &Installedfonts);
            return Createfont(Name, Fontsize);
        }
    }
    namespace Images
    {
        template <bool isRGB>   // NOTE(tcn): Windows wants to use BGR for bitmaps, probably some Win16 reasoning.
        std::pair<BITMAP, HBITMAP> Createimage(const uint8_t *Pixelbuffer, const POINT Size, size_t Bitsperpixel = 24)
        {
            assert(Size.x); assert(Size.y); assert(Pixelbuffer);
            BITMAPINFO BMI{ sizeof(BITMAPINFOHEADER) };
            const size_t Pixelsize = Bitsperpixel / 8;
            uint8_t *DIBPixels{};

            // Windows likes to draw bitmaps upside down, so negative height.
            BMI.bmiHeader.biSizeImage = Size.y * Size.x * Pixelsize;
            BMI.bmiHeader.biBitCount = Bitsperpixel;
            BMI.bmiHeader.biHeight = (-1) * Size.y;
            BMI.bmiHeader.biCompression = BI_RGB;
            BMI.bmiHeader.biWidth = Size.x;
            BMI.bmiHeader.biPlanes = 1;

            // Allocate the bitmap in system-memory.
            const auto Bitmap = CreateDIBSection(NULL, &BMI, DIB_RGB_COLORS, (void **)&DIBPixels, NULL, 0);
            std::memcpy(Bitmap, Pixelbuffer, Size.x * Size.y * Pixelsize);

            // Swap R and B channels.
            if constexpr (isRGB)
            {
                // TODO(tcn): Maybe some kind of SSE optimization here?
                for (size_t i = 0; i < Size.x * Size.y; i += Pixelsize)
                {
                    std::swap(DIBPixels[i], DIBPixels[i + 2]);
                }
            }

            // Re-associate the memory with our bitmap.
            SetDIBits(NULL, Bitmap, 0, Size.y, DIBPixels, &BMI, DIB_RGB_COLORS);

            BITMAP Result;
            GetObjectA(Bitmap, sizeof(BITMAP), &Result);
            return { Result, Bitmap };
        }
    }
}

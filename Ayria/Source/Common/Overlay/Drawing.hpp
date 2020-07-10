/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-07-06
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>

namespace Paint
{
    struct Pen_t
    {
        static std::unordered_map<int, HPEN, decltype(FNV::Hash), decltype(FNV::Equal)> Pens;
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

    #define COMMON HDC Devicecontext, [[maybe_unused]] POINT Position, [[maybe_unused]] COLORREF Colour
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

            const float X0 = Position.x + Rounding * std::cosf(static_cast<float>((Angles.x + Angles.y) * std::numbers::pi / 180.0f));
            const float Y0 = Position.y + Rounding * std::sinf(static_cast<float>((Angles.x + Angles.y) * std::numbers::pi / 180.0f));
            const float X1 = Position.x + Rounding * std::cosf(static_cast<float>(Angles.x * std::numbers::pi / 180.0f));
            const float Y1 = Position.y + Rounding * std::sinf(static_cast<float>(Angles.x * std::numbers::pi / 180.0f));

            SetArcDirection(Devicecontext, AD_COUNTERCLOCKWISE);
            Arc(Devicecontext, Position.x - Rounding, Position.y - Rounding,
                Position.x + Rounding, Position.y + Rounding,
                (int)X0, (int)Y0, (int)X1, (int)Y1);
        }
        inline void Pie(COMMON, POINTFLOAT Angles, int Rounding, int Linewidth, COLORREF Background)
        {
            const Pen_t Pen(Devicecontext, Linewidth, Colour);
            SetDCBrushColor(Devicecontext, Background);

            const float X0 = Position.x + Rounding * std::cosf(static_cast<float>((Angles.x + Angles.y) * std::numbers::pi / 180.0f));
            const float Y0 = Position.y + Rounding * std::sinf(static_cast<float>((Angles.x + Angles.y) * std::numbers::pi / 180.0f));
            const float X1 = Position.x + Rounding * std::cosf(static_cast<float>(Angles.x * std::numbers::pi / 180.0f));
            const float Y1 = Position.y + Rounding * std::sinf(static_cast<float>(Angles.x * std::numbers::pi / 180.0f));

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
            for (size_t i = 0; i < Points.size(); ++i)
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
            for (size_t i = 0; i < Points.size(); ++i)
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
            for (size_t i = 0; i < Points.size(); ++i)
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
                return { P.x, P.y,
                         static_cast<COLOR16>((C & 0xFF) << 8),
                         static_cast<COLOR16>((C & 0xFF00) << 8),
                         static_cast<COLOR16>((C & 0xFF0000) << 8),
                         static_cast<COLOR16>((C & 0xFF000000) << 8) };
            };
            const auto Array = (TRIVERTEX *)alloca(Points.size() * sizeof(TRIVERTEX));
            for (size_t i = 0; i < Points.size(); ++i) Array[i] = toVertex(Points[i], Colours[i]);

            // Malformed quad, draw as two triangles.
            if (Points.size() == 4)
            {
                GRADIENT_TRIANGLE Index[2] = { { 0, 1, 3 },
                                               { 2, 1, 3 } };
                GradientFill(Devicecontext, Array, Points.size(), Index, 2, GRADIENT_FILL_TRIANGLE);
            }
            else
            {
                // Assume that the points are in order.
                const auto Index = (GRADIENT_TRIANGLE *)alloca(Points.size() / 3 * sizeof(GRADIENT_TRIANGLE));
                for (ULONG i = 0; i < Points.size() / 3; ++i) Index[i] = { i, i + 1, i + 2 };
                GradientFill(Devicecontext, Array, Points.size(), Index, Points.size() / 3, GRADIENT_FILL_TRIANGLE);
            }
        }
    }

    namespace Text
    {
        inline void Transparant(COMMON, HFONT Fonthandle, std::wstring_view String)
        {
            SetTextColor(Devicecontext, Colour);
            SelectObject(Devicecontext, Fonthandle);
            ExtTextOutW(Devicecontext, Position.x, Position.y, ETO_NUMERICSLATIN, NULL, String.data(), String.size(), NULL);
        }
        inline void Opaque(COMMON, HFONT Fonthandle, std::wstring_view String, COLORREF Background)
        {
            SetTextColor(Devicecontext, Colour);
            SetBkColor(Devicecontext, Background);
            SelectObject(Devicecontext, Fonthandle);
            ExtTextOutW(Devicecontext, Position.x, Position.y, ETO_NUMERICSLATIN | ETO_OPAQUE, NULL, String.data(), String.size(), NULL);
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

    inline HFONT getDefaultfont()
    {
        static const auto Default = Fonts::Createfont("Consolas", 16);
        return Default;
    }
    inline POINT getTextsize(HDC Devicecontext, std::string_view Text)
    {
        SIZE Textsize;
        const auto String = toWide(Text);

        if (GetTextExtentPoint32W(Devicecontext, String.c_str(), int(String.size()), &Textsize))
            return { Textsize.cx, Textsize.cy };
        return {};
    }
}
namespace Images
{
    template <bool isRGB>   // NOTE(tcn): Windows wants to use BGR for bitmaps, probably some Win16 reasoning.
    std::pair<BITMAP, HBITMAP> Createimage(const uint8_t *Pixelbuffer, const POINT Size, size_t Bitsperpixel = 24)
    {
        assert(Size.x);
        assert(Size.y);
        assert(Pixelbuffer);
        BITMAPINFO BMI{ sizeof(BITMAPINFOHEADER) };
        const size_t Pixelsize = Bitsperpixel / 8;
        uint8_t *DIBPixels{};

        // Windows likes to draw bitmaps upside down, so negative height.
        BMI.bmiHeader.biSizeImage = Size.y * Size.x * Pixelsize;
        BMI.bmiHeader.biBitCount = static_cast<WORD>(Bitsperpixel);
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
        SetDIBits(NULL, Bitmap, 0, static_cast<UINT>(Size.y), DIBPixels, &BMI, DIB_RGB_COLORS);
        BITMAP Result;
        GetObjectA(Bitmap, sizeof(BITMAP), &Result);
        return { Result, Bitmap };
    }
}

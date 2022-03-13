/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-07-20
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>
#include "../Datatypes.hpp"

// Windows only module.
static_assert(Build::isWindows, "Renderer is only available on Windows for now.");

// Unified format.
#pragma region Datatypes
#pragma pack(push, 1)

struct rgb_t { uint8_t r, g, b; };
struct Color_t : rgb_t
{
    uint8_t a;

    constexpr Color_t() : rgb_t() { a = 0xFF; }
    constexpr Color_t(rgb_t RGB) : rgb_t(RGB) { a = 0xFF; };
    constexpr Color_t(uint8_t R, uint8_t G, uint8_t B, uint8_t A = 0xFF) : rgb_t{ R, G, B }, a(A) {}
    constexpr Color_t(COLORREF RGBA) { r = RGBA & 0xFF; g = (RGBA >> 8) & 0xFF;  b = (RGBA >> 16) & 0xFF; a = (RGBA >> 24) & 0xFF; if (a == 0) a = 0xFF; }
    constexpr Color_t(uint32_t RGBA) { b = RGBA & 0xFF; g = (RGBA >> 8) & 0xFF;  r = (RGBA >> 16) & 0xFF; a = (RGBA >> 24) & 0xFF; if (a == 0) a = 0xFF; }

    static constexpr rgb_t Blend(rgb_t Source, rgb_t Overlay, uint8_t Opacity)
    {
        #define Channel(x, y, z) x = (x * z + y * (0xFF - z)) / 0xFF;
        Channel(Source.r, Overlay.r, Opacity);
        Channel(Source.g, Overlay.g, Opacity);
        Channel(Source.b, Overlay.b, Opacity);
        #undef Channel

        return Source;
    }
    static constexpr rgb_t Blend(rgb_t Source, rgb_t Overlay, float Opacity)
    {
        return Blend(Source, Overlay, uint8_t(Opacity * 0xFF));
    }
    static constexpr Color_t Blend(rgb_t Source, Color_t Overlay)
    {
        return Blend(Source, static_cast<rgb_t>(Overlay), Overlay.a);
    }
    void Blend(rgb_t Overlay, uint8_t Opacity)
    {
        *this = Blend(static_cast<rgb_t>(*this), Overlay, Opacity);
    }
    void Blend(Color_t Overlay)
    {
        *this = Blend(static_cast<rgb_t>(*this), Overlay);
    }

    constexpr operator rgb_t() const { return { r, g, b }; }
    constexpr operator RGBTRIPLE() const { return { b, g, r }; }
    constexpr operator RGBQUAD() const { return { b, g, r, a }; }
    constexpr operator COLORREF() const { return r | (g << 8U) | (b << 16U); }
};

#pragma pack(pop)
#pragma endregion

namespace Graphics
{
    // General font management, no cache as Windows should handle that..
    inline HFONT Createfont(const std::wstring &Name, int8_t Fontsize)
    {
        return CreateFontW(Fontsize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, Name.c_str());
    }
    inline HFONT Createfont(const std::wstring &Name, int8_t Fontsize, Blob_view Data)
    {
        DWORD Fontcount{};
        (void)AddFontMemResourceEx((void *)Data.data(), (DWORD)Data.size(), NULL, &Fontcount);
        return Createfont(Name, Fontsize);
    }
    inline HFONT getDefaultfont(int8_t Fontsize = 0)
    {
        if (Fontsize == 0)
        {
            CONSOLE_FONT_INFO ConsoleFontInfo;
            GetCurrentConsoleFont(GetStdHandle(STD_OUTPUT_HANDLE), false, &ConsoleFontInfo);
            const auto Coord = GetConsoleFontSize(GetStdHandle(STD_OUTPUT_HANDLE), ConsoleFontInfo.nFont);

            Fontsize = Coord.Y - 2;
        }
        return Createfont(L"Consolas", Fontsize);
    }

    // RTTI helpers for the GDI state.
    struct Backgroundcolor_t
    {
        COLORREF Previouscolor;
        HDC DC;

        explicit Backgroundcolor_t(HDC Devicecontext, COLORREF Color) : DC(Devicecontext)
        {
            Previouscolor = SetBkColor(DC, Color);
        }
        ~Backgroundcolor_t()
        {
            SetBkColor(DC, Previouscolor);
        }
    };
    struct Textcolor_t
    {
        COLORREF Previouscolor;
        HDC DC;

        explicit Textcolor_t(HDC Devicecontext, COLORREF Color) : DC(Devicecontext)
        {
            Previouscolor = SetTextColor(DC, Color);
        }
        ~Textcolor_t()
        {
            SetTextColor(DC, Previouscolor);
        }
    };
    struct Brush_t
    {
        HGDIOBJ Previousobject;
        HDC DC;

        explicit Brush_t(HDC Devicecontext, COLORREF Color) : DC(Devicecontext)
        {
            const auto TMP = CreateSolidBrush(Color);
            Previousobject = SelectObject(DC, TMP);
        }
        ~Brush_t()
        {
            DeleteObject(SelectObject(DC, Previousobject));
        }
    };
    struct Font_t
    {
        HGDIOBJ Previousobject;
        HDC DC;

        explicit Font_t(HDC Devicecontext, HFONT Font) : DC(Devicecontext)
        {
            Previousobject = SelectObject(DC, Font);
        }
        ~Font_t()
        {
            DeleteObject(SelectObject(DC, Previousobject));
        }
    };
    struct Pen_t
    {
        HGDIOBJ Previousobject;
        COLORREF Previouscolor;
        HDC DC;

        explicit Pen_t(HDC Devicecontext, int Linewidth, COLORREF Color) : DC(Devicecontext)
        {
            Previouscolor = GetDCPenColor(DC);
            const auto TMP = CreatePen(PS_SOLID, Linewidth, Color);
            Previousobject = SelectObject(DC, TMP);
        }
        ~Pen_t()
        {
            DeleteObject(SelectObject(DC, Previousobject));
            SetDCPenColor(DC, Previouscolor);
        }
    };

    // Wrapper around the GDI context.
    struct Renderobject_t
    {
        HDC Devicecontext;
        Renderobject_t(HDC Context) : Devicecontext(Context) {}

        // Optional values are used as booleans for Mesh(). Outline can also be Foreground or text color.
        virtual void Render(std::optional<Color_t> Outline, std::optional<Color_t> Background) = 0;
    };
    struct Renderer_t
    {
        HDC Devicecontext;
        HRGN Clipping{};

        // Accesses thread local storage.
        ~Renderer_t();

        // Needs to be explicitly instantiated by an overlay.
        explicit Renderer_t(HDC Context) : Devicecontext(Context) {}
        explicit Renderer_t(HDC Context, vec4f Boundingbox) : Devicecontext(Context)
        {
            // GDI's clipping is a little odd..
            Clipping = CreateRectRgn(Boundingbox.x, Boundingbox.y, Boundingbox.z, 2 + (int)Boundingbox.w);
            SelectClipRgn(Devicecontext, Clipping);
        }

        // Create renderobjects based on the users needs, only valid until the next call.
        std::shared_ptr<Renderobject_t> Line(vec2i Start, vec2i Stop, uint8_t Linewidth = 1) const noexcept;
        std::shared_ptr<Renderobject_t> Ellipse(vec2i Position, vec2i Size, uint8_t Linewidth = 1) const noexcept;
        std::shared_ptr<Renderobject_t> Path(const std::vector<vec2i> &Points, uint8_t Linewidth = 1) const noexcept;
        std::shared_ptr<Renderobject_t> Gradientrect(vec2i Position, vec2i Size, bool Vertical = false) const noexcept;
        std::shared_ptr<Renderobject_t> Polygon(const std::vector<vec2i> &Points, uint8_t Linewidth = 1) const noexcept;
        std::shared_ptr<Renderobject_t> Arc(vec2i Position, vec2i Angles, uint8_t Rounding, uint8_t Linewidth = 1) const noexcept;
        std::shared_ptr<Renderobject_t> Rectangle(vec2i Position, vec2i Size, uint8_t Rounding = 0, uint8_t Linewidth = 1) const noexcept;
        std::shared_ptr<Renderobject_t> Mesh(const std::vector<vec2i> &Points, const std::vector<Color_t> &Colors, uint8_t Linewidth = 1) const noexcept;

        std::shared_ptr<Renderobject_t> Text(vec2i Position, const std::wstring &String, HFONT Font = getDefaultfont()) const noexcept;
        std::shared_ptr<Renderobject_t> Textcentered(vec4f Boundingbox, const std::wstring &String, HFONT Font = getDefaultfont()) const noexcept;
    };
}

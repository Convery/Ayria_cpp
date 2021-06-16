/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-06-13
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>
#include <Global.hpp>

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
        AddFontMemResourceEx((void *)Data.data(), (DWORD)Data.size(), NULL, &Fontcount);
        return Createfont(Name, Fontsize);
    }
    inline HFONT getDefaultfont(int8_t Fontsize)
    {
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

        // Needs to be explicitly instantiated by an overlay.
        explicit Renderer_t(HDC Context) : Devicecontext(Context) {}
        explicit Renderer_t(HDC Context, vec4f Boundingbox) : Devicecontext(Context)
        {
            // GDI's clipping is a little odd..
            Clipping = CreateRectRgn(Boundingbox.x, Boundingbox.y, Boundingbox.z, 1 + (int)Boundingbox.w);
            SelectClipRgn(Devicecontext, Clipping);
        }
        ~Renderer_t();

        // Create renderobjects based on the users needs, only valid until the next call.
        Renderobject_t *Line(vec2f Start, vec2f Stop, uint8_t Linewidth = 1) const noexcept;
        Renderobject_t *Ellipse(vec2f Position, vec2f Size, uint8_t Linewidth = 1) const noexcept;
        Renderobject_t *Path(const std::vector<vec2f> &Points, uint8_t Linewidth = 1) const noexcept;
        Renderobject_t *Gradientrect(vec2f Position, vec2f Size, bool Vertical = false) const noexcept;
        Renderobject_t *Polygon(const std::vector<vec2f> &Points, uint8_t Linewidth = 1) const noexcept;
        Renderobject_t *Arc(vec2f Position, vec2f Angles, uint8_t Rounding, uint8_t Linewidth = 1) const noexcept;
        Renderobject_t *Rectangle(vec2f Position, vec2f Size, uint8_t Rounding = 0, uint8_t Linewidth = 1) const noexcept;
        Renderobject_t *Mesh(const std::vector<vec2f> &Points, const std::vector<Color_t> &Colors, uint8_t Linewidth = 1) const noexcept;

        Renderobject_t *Text(vec2f Position, const std::wstring &String, HFONT Font = getDefaultfont(16)) const noexcept;
        Renderobject_t *Textcentered(vec4f Boundingbox, const std::wstring &String, HFONT Font = getDefaultfont(16)) const noexcept;
    };
}

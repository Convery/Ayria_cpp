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
    // Add an enmbedded font to the library.
    inline void Createfont(const std::wstring &Name, Blob_view Data)
    {
        DWORD Fontcount{};
        (void)AddFontMemResourceEx((void *)Data.data(), (DWORD)Data.size(), NULL, &Fontcount);
    }

    // Wrapper around the GDI context.
    struct Renderobject_t
    {
        HDC Devicecontext;
        Renderobject_t(HDC Context) : Devicecontext(Context) {}

        // Optional values are used as booleans for Mesh(). Outline can also be Foreground or text color.
        virtual void Render(std::optional<Color_t> Outline, std::optional<Color_t> Background) = 0;

        virtual ~Renderobject_t() = default;
    };
    struct Renderer_t
    {
        HDC Devicecontext;
        HRGN Clipping{};

        // Clipping is only initialized if we created it.
        ~Renderer_t()
        {
            if (Clipping) DeleteObject(Clipping);
        }

        // Needs to be explicitly instantiated by an overlay.
        explicit Renderer_t(HDC Context) : Devicecontext(Context)
        {
        }
        explicit Renderer_t(HDC Context, HRGN Region) : Devicecontext(Context)
        {
            SelectClipRgn(Devicecontext, Region);
        }
        explicit Renderer_t(HDC Context, vec4i Boundingbox) : Devicecontext(Context)
        {
            Clipping = CreateRectRgn(Boundingbox.x, Boundingbox.y, Boundingbox.z, Boundingbox.w);
            SelectClipRgn(Devicecontext, Clipping);
        }

        // Create renderobjects based on the users needs, only valid until the next call.
        std::unique_ptr<Renderobject_t> Line(vec2i Start, vec2i Stop, uint8_t Linewidth = 1) const noexcept;
        std::unique_ptr<Renderobject_t> Ellipse(vec2i Position, vec2i Size, uint8_t Linewidth = 1) const noexcept;
        std::unique_ptr<Renderobject_t> Path(const std::vector<vec2i> &Points, uint8_t Linewidth = 1) const noexcept;
        std::unique_ptr<Renderobject_t> Gradientrect(vec2i Position, vec2i Size, bool Vertical = false) const noexcept;
        std::unique_ptr<Renderobject_t> Polygon(const std::vector<vec2i> &Points, uint8_t Linewidth = 1) const noexcept;
        std::unique_ptr<Renderobject_t> Text(vec2i Position, const std::wstring &String, HFONT Font = NULL) const noexcept;
        std::unique_ptr<Renderobject_t> Arc(vec2i Position, vec2i Angles, uint8_t Rounding, uint8_t Linewidth = 1) const noexcept;
        std::unique_ptr<Renderobject_t> Textcentered(vec4i Boundingbox, const std::wstring &String, HFONT Font = NULL) const noexcept;
        std::unique_ptr<Renderobject_t> Rectangle(vec2i Position, vec2i Size, uint8_t Rounding = 0, uint8_t Linewidth = 1) const noexcept;
        std::unique_ptr<Renderobject_t> Mesh(const std::vector<vec2i> &Points, const std::vector<Color_t> &Colors, uint8_t Linewidth = 1) const noexcept;
    };
}

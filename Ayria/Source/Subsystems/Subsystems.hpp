/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-12-17
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>

// Common data structures.
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
        return Blend(Source, Overlay, Overlay.a);
    }
    void Blend(rgb_t Overlay, uint8_t Opacity)
    {
        *this = Blend(*this, Overlay, Opacity);
    }
    void Blend(Color_t Overlay)
    {
        *this = Blend(*this, Overlay);
    }

    constexpr operator rgb_t() const { return { r, g, b }; }
    constexpr operator RGBTRIPLE() const { return { b, g, r }; }
    constexpr operator RGBQUAD() const { return { b, g, r, a }; }
    constexpr operator COLORREF() const { return r | (g << 8U) | (b << 16U); }
};

#pragma pack(pop)
#pragma endregion

// Subsystems that depend on the datatypes.
#include "Pluginloader/Pluginloader.hpp"
#include "Overlay/Graphics.hpp"
#include "Overlay/Overlay.hpp"
#include "Console/Console.hpp"

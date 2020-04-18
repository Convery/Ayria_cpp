/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-04-18
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>
#include <type_traits>

namespace Graphics
{
    /*
        This format is for storage, so we use bfloat16 rather than I3E 754
        for faster conversion as actual arithmetic will be done on float32.
        For this reason we do not check for NaN, INF, and other such specs.
    */
    struct float16_t
    {
        uint16_t Internal;

        float16_t() : Internal(0){}
        float16_t(float Value)
        {
            if (std::fabs(Value) < std::numeric_limits<float>::min()) [[unlikely]]
            {
                Internal = std::signbit(Value) << 15;
            }
            else
            {
                // To get around helpful compilers casting.
                uint32_t Raw = *(uint32_t *)&Value;
                Raw += 0x7FFF + ((Raw >> 16) & 1);
                Internal = static_cast<uint16_t>(Raw >> 16);
            }
        }
        operator float() const
        {
            uint32_t Temp = Internal << 16;
            return *(float *)&Temp;
        }

        template<typename T> requires std::is_integral<T>::value
        float16_t(T Value) : float16_t(static_cast<float>(Value)) {}
        template<typename T> requires std::is_integral<T>::value
        operator T() { return static_cast<T>(static_cast<float>(*this)); }
    };

    // For easy conversion between formats.
    struct colour_t
    {
        union { struct { uint8_t R, G, B, A; }; uint32_t Raw; };

        constexpr void Blend(colour_t b)
        {
            this->R += int32_t((((b.R - this->R) * b.A))) >> 8;
            this->G += int32_t((((b.G - this->G) * b.A))) >> 8;
            this->B += int32_t((((b.B - this->B) * b.A))) >> 8;
        }
        constexpr colour_t Blend(colour_t a, colour_t b)
        {
            a.R += int32_t((((b.R - a.R) * b.A))) >> 8;
            a.G += int32_t((((b.G - a.G) * b.A))) >> 8;
            a.B += int32_t((((b.B - a.B) * b.A))) >> 8;
            return a;
        }
        constexpr uint8_t f2b(float Value) { return  static_cast<uint8_t>(Value * (Value <= 1.0f * 255)); }

        constexpr colour_t() : Raw(0xFF) {};
        constexpr colour_t(uint32_t Hex) : Raw(Hex) {};
        constexpr colour_t(uint8_t r, uint8_t g, uint8_t b) : R(r), G(g), B(b), A(0xFF) {};
        constexpr colour_t(uint8_t r, uint8_t g, uint8_t b, uint8_t a) : R(r), G(g), B(b), A(a) {};
        constexpr colour_t(float r, float g, float b) : R(f2b(r)), G(f2b(g)), B(f2b(b)), A(0xFF) {};
        constexpr colour_t(float r, float g, float b, float a) : R(f2b(r)), G(f2b(g)), B(f2b(b)), A(f2b(a)) {};
    };

    // No vec3 because Z-order should be implied by the draw-calls.
    using point2_t = union { struct { int16_t x, y; }; uint32_t Raw; };
    using vec2_t = union { struct { float16_t x, y; }; uint32_t Raw; };
    using point4_t = union { struct { int16_t x0, x1, y0, y1; }; uint64_t Raw; };
    using vec4_t = union { struct { float16_t x0, x1, y0, y1; }; uint64_t Raw; };





    struct Window
    {
        /*
            Current style config
            Elements
        */


    };
}



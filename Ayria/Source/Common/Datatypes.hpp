/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-08-19
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>

// Common data structures.
#pragma region Datatypes
#pragma pack(push, 1)

// See frameworks like Tensorflow for improvement ideas of bfloat16.
struct bfloat16_t
{
    uint16_t Value;

    using FP = union { uint32_t I; float F; };
    static bfloat16_t Round(float Input)
    {
        FP Value; Value.F = Input;

        // Special cases.
        if ((Value.I & 0xFF800000) == 0) return uint16_t(0);
        if ((Value.I & 0xFF800000) == 0x80000000) return uint16_t(0x8000);

        const auto LSB = (Value.I >> 16) & 1;
        return uint16_t((Value.I + 0x7FFF + LSB) >> 16);
    }
    static bfloat16_t Truncate(float Input)
    {
        if (std::fabs(Input) < std::numeric_limits<float>::min())
            return uint16_t(std::signbit(Input) ? 0x8000 : 0);

        return uint16_t((*(uint32_t *)&Input) & 0xFFFF);
    }

    bfloat16_t(uint16_t Input) : Value(Input) {}
    bfloat16_t(float Input)
    {
        *this = Round(Input);
    }
    bfloat16_t(int Input)
    {
        *this = Round(float(Input));
    }
    bfloat16_t() : Value(0) {}

    template<typename T> operator T() const { FP Output; Output.I = Value << 16; return T(Output.F); }
    bfloat16_t &operator+=(const bfloat16_t &Right) { const float L = *this, R = Right; *this = (L + R); return *this; }
    bfloat16_t &operator-=(const bfloat16_t &Right) { const float L = *this, R = Right; *this = (L - R); return *this; }
    bfloat16_t &operator*=(const bfloat16_t &Right) { const float L = *this, R = Right; *this = (L * R); return *this; }
    bfloat16_t &operator/=(const bfloat16_t &Right) { const float L = *this, R = Right; *this = (L / R); return *this; }
    friend bfloat16_t operator+(bfloat16_t Left, const bfloat16_t &Right) { Left += Right; return Left; }
    friend bfloat16_t operator-(bfloat16_t Left, const bfloat16_t &Right) { Left -= Right; return Left; }
    friend bfloat16_t operator*(bfloat16_t Left, const bfloat16_t &Right) { Left *= Right; return Left; }
    friend bfloat16_t operator/(bfloat16_t Left, const bfloat16_t &Right) { Left /= Right; return Left; }
    bool operator!=(const bfloat16_t &Right) { return 0 != (Value - Right.Value); }
    auto operator<=>(const bfloat16_t &) const = default;
};

struct vec2_t
{
    bfloat16_t x, y;

    vec2_t() { x = y = 0; }
    vec2_t(int X, int Y) : x(X), y(Y) {}
    vec2_t(float X, int Y) : x(X), y(Y) {}
    vec2_t(int X, float Y) : x(X), y(Y) {}
    vec2_t(float X, float Y) : x(X), y(Y) {}

    template<typename T> operator T() const { return { x, y }; }
    operator bool() const { return !!(x.Value + y.Value); }

    vec2_t &operator+=(const vec2_t &Right) { x += Right.x; y += Right.y; return *this; }
    vec2_t &operator-=(const vec2_t &Right) { x -= Right.x; y -= Right.y; return *this; }
    vec2_t &operator*=(const bfloat16_t &Right) { x *= Right; y *= Right; return *this; }
    friend vec2_t operator+(vec2_t Left, const vec2_t &Right) { Left += Right; return Left; }
    friend vec2_t operator-(vec2_t Left, const vec2_t &Right) { Left -= Right; return Left; }
    friend vec2_t operator*(vec2_t Left, const bfloat16_t &Right) { Left *= Right; return Left; }
    bool operator!=(const vec2_t &Right) { return 0 != ((x.Value + y.Value) - (Right.x.Value + Right.y.Value)); }
    auto operator<=>(const vec2_t &) const = default;
};
struct vec3_t : vec2_t
{
    bfloat16_t z;

    vec3_t() { x = y = z = 0; }
    vec3_t(vec2_t XY) : vec2_t(XY) { z = 0; }
    vec3_t(vec2_t XY, int Z) : vec2_t(XY), z(Z) {}
    vec3_t(vec2_t XY, float Z) : vec2_t(XY), z(Z) {}

    operator bool() const { return !!(x.Value + y.Value + z.Value); }
    template<typename T> operator T() const { return { x, y, z }; }
    operator vec2_t() const { return { float(x), float(y) }; }

    vec3_t &operator+=(const vec3_t &Right) { x += Right.x; y += Right.y; z += Right.z; return *this; }
    vec3_t &operator-=(const vec3_t &Right) { x -= Right.x; y -= Right.y; z -= Right.z; return *this; }
    vec3_t &operator*=(const bfloat16_t &Right) { x *= Right; y *= Right; z *= Right; return *this; }
    friend vec3_t operator*(vec3_t Left, const bfloat16_t &Right) { Left *= Right; return Left; }
    friend vec3_t operator+(vec3_t Left, const vec3_t &Right) { Left += Right; return Left; }
    friend vec3_t operator-(vec3_t Left, const vec3_t &Right) { Left -= Right; return Left; }
    auto operator<=>(const vec3_t &) const = default;
};
struct vec4_t : vec3_t
{
    bfloat16_t w;

    vec4_t() { x = y = z = w = 0; }
    vec4_t(vec3_t XYZ) : vec3_t(XYZ) { w = 0; }
    vec4_t(vec2_t XY) : vec3_t(XY) { z = w = 0; }
    vec4_t(vec3_t XYZ, int W) : vec3_t(XYZ), w(W) {}
    vec4_t(vec3_t XYZ, float W) : vec3_t(XYZ), w(W) {}
    vec4_t(vec2_t XY, vec2_t WH) : vec3_t(XY) { z = x + WH.x; w = y + WH.y; }

    template<typename T> operator T() const { return { x, y, z, w }; }
    operator vec3_t() const { return { { float(x), float(y) }, float(z) }; }
    operator bool() const { return !!(x.Value + y.Value + z.Value + w.Value); }

    vec4_t &operator+=(const vec4_t &Right) { x += Right.x; y += Right.y; z += Right.z; w += Right.w; return *this; }
    vec4_t &operator-=(const vec4_t &Right) { x -= Right.x; y -= Right.y; z -= Right.z; w -= Right.w; return *this; }
    vec4_t &operator*=(const bfloat16_t &Right) { x *= Right; y *= Right; z *= Right; w *= Right; return *this; }
    friend vec4_t operator*(vec4_t Left, const bfloat16_t &Right) { Left *= Right; return Left; }
    friend vec4_t operator+(vec4_t Left, const vec4_t &Right) { Left += Right; return Left; }
    friend vec4_t operator-(vec4_t Left, const vec4_t &Right) { Left -= Right; return Left; }
    auto operator<=>(const vec4_t &) const = default;
};

struct rgb_t { uint8_t r, g, b; };
struct Color_t : rgb_t
{
    uint8_t a;

    Color_t() : rgb_t() { a = 0xFF; }
    Color_t(rgb_t RGB) : rgb_t(RGB) { a = 0xFF; };
    Color_t(COLORREF RGBA) : rgb_t(*(rgb_t *)&RGBA) { a = (RGBA >> 24) & 0xFF; }
    Color_t(uint8_t R, uint8_t G, uint8_t B, uint8_t A = 0xFF) : rgb_t{ R, G, B }, a(A) {}
    Color_t(float R, float G, float B, float A = 1.0f) :
        rgb_t{ uint8_t(R * 0xFF), uint8_t(G * 0xFF), uint8_t(B * 0xFF) }, a(A * 0xFF) {}

    static rgb_t Blend(rgb_t Source, rgb_t Overlay, uint8_t Opacity)
    {
        #define Channel(x, y, z) x = (x * z + y * (0xFF - z)) / 0xFF;
        Channel(Source.r, Overlay.r, Opacity);
        Channel(Source.g, Overlay.g, Opacity);
        Channel(Source.b, Overlay.b, Opacity);
        #undef Channel

        return Source;
    }
    static rgb_t Blend(rgb_t Source, rgb_t Overlay, float Opacity)
    {
        return Blend(Source, Overlay, uint8_t(Opacity * 0xFF));
    }
    static Color_t Blend(rgb_t Source, Color_t Overlay)
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

    operator rgb_t() const { return { r, g, b }; }
    operator RGBTRIPLE() const { return { b, g, r }; }
    operator RGBQUAD() const { return { b, g, r, a }; }
    operator COLORREF() const { return r | (g << 8U) | (b << 16U) | (a << 24U); }
};

#pragma pack(pop)
#pragma endregion

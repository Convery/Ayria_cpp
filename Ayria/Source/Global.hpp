/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-07-10
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>

// Common data structures.
#pragma region Datatypes
#pragma pack(push, 1)

// NOTE(tcn): auto operator<=>(const T &) const = default; is borked on MSVC 16.7.3..

// See ML-frameworks like Tensorflow for optimisation ideas of bfloat16.
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

    bfloat16_t() : Value(0) {}
    template<typename T> bfloat16_t(T Input)
    {
        if constexpr (std::is_same_v<T, uint16_t>) Value = Input;
        else if constexpr (std::is_floating_point_v<T>) *this = Round(Input);
        else *this = Round(float(Input));
    }
    template<typename T> operator T() const { FP Output; Output.I = Value << 16; return T(Output.F); }

    bool operator!=(const bfloat16_t &Right) const { return !operator==(Right); }
    bool operator==(const bfloat16_t &Right) const { return Value == Right.Value; }
    bool operator<(const bfloat16_t &Right)  const { return float(*this) < float(Right); }
    bool operator>(const bfloat16_t &Right)  const { return float(*this) > float(Right); }
    bool operator<=(const bfloat16_t &Right) const { return float(*this) <= float(Right); }
    bool operator>=(const bfloat16_t &Right) const { return float(*this) >= float(Right); }

    friend bfloat16_t operator+(bfloat16_t Left, const bfloat16_t &Right) { Left += Right; return Left; }
    friend bfloat16_t operator-(bfloat16_t Left, const bfloat16_t &Right) { Left -= Right; return Left; }
    friend bfloat16_t operator*(bfloat16_t Left, const bfloat16_t &Right) { Left *= Right; return Left; }
    friend bfloat16_t operator/(bfloat16_t Left, const bfloat16_t &Right) { Left /= Right; return Left; }

    bfloat16_t &operator+=(const bfloat16_t &Right) { const float L = *this, R = Right; *this = (L + R); return *this; }
    bfloat16_t &operator-=(const bfloat16_t &Right) { const float L = *this, R = Right; *this = (L - R); return *this; }
    bfloat16_t &operator*=(const bfloat16_t &Right) { const float L = *this, R = Right; *this = (L * R); return *this; }
    bfloat16_t &operator/=(const bfloat16_t &Right) { const float L = *this, R = Right; *this = (L / R); return *this; }
};
struct vec2_t
{
    bfloat16_t x{}, y{};

    vec2_t() = default;
    operator bool() const { return !!(x.Value + y.Value); }
    template<typename T> operator T() const { return { x, y }; }
    template<typename T, typename U> vec2_t(T X, U Y) : x(X), y(Y) {}

    bool operator!=(const vec2_t &Right) const { return !operator==(Right); }
    bool operator==(const vec2_t &Right) const { return x == Right.x && y == Right.y; }
    bool operator<(const vec2_t &Right)  const { return (x < Right.x) && (y < Right.y); }
    bool operator>(const vec2_t &Right)  const { return (x > Right.x) && (y > Right.y); }
    bool operator<=(const vec2_t &Right) const { return (x <= Right.x) && (y <= Right.y); }
    bool operator>=(const vec2_t &Right) const { return (x >= Right.x) && (y >= Right.y); }

    friend vec2_t operator+(vec2_t Left, const vec2_t &Right) { Left += Right; return Left; }
    friend vec2_t operator-(vec2_t Left, const vec2_t &Right) { Left -= Right; return Left; }
    friend vec2_t operator*(vec2_t Left, const bfloat16_t &Right) { Left *= Right; return Left; }

    vec2_t &operator*=(const bfloat16_t &Right) { x *= Right; y *= Right; return *this; }
    vec2_t &operator+=(const vec2_t &Right) { x += Right.x; y += Right.y; return *this; }
    vec2_t &operator-=(const vec2_t &Right) { x -= Right.x; y -= Right.y; return *this; }
};
struct vec3_t
{
    bfloat16_t x{}, y{}, z{};

    vec3_t() = default;
    template<typename T> operator T() const { return { x, y, z }; }
    operator bool() const { return !!(x.Value + y.Value + z.Value); }
    template<typename T = int> vec3_t(vec2_t XY, T Z = 0) : x(XY.x), y(XY.y), z(Z) {}
    template<typename T, typename U, typename V> vec3_t(T X, U Y, V Z) : x(X), y(Y), z(Z) {}

    bool operator!=(const vec3_t &Right) const { return !operator==(Right); }
    bool operator==(const vec3_t &Right) const { return x == Right.x && y == Right.y && z == Right.z; }
    bool operator<(const vec3_t &Right)  const { return (x < Right.x) && (y < Right.y) && (z < Right.z); }
    bool operator>(const vec3_t &Right)  const { return (x > Right.x) && (y > Right.y) && (z > Right.z); }
    bool operator<=(const vec3_t &Right) const { return (x <= Right.x) && (y <= Right.y) && (z <= Right.z); }
    bool operator>=(const vec3_t &Right) const { return (x >= Right.x) && (y >= Right.y) && (z >= Right.z); }

    friend vec3_t operator+(vec3_t Left, const vec3_t &Right) { Left += Right; return Left; }
    friend vec3_t operator-(vec3_t Left, const vec3_t &Right) { Left -= Right; return Left; }
    friend vec3_t operator*(vec3_t Left, const bfloat16_t &Right) { Left *= Right; return Left; }

    vec3_t &operator*=(const bfloat16_t &Right) { x *= Right; y *= Right; z *= Right; return *this; }
    vec3_t &operator+=(const vec3_t &Right) { x += Right.x; y += Right.y; z += Right.z; return *this; }
    vec3_t &operator-=(const vec3_t &Right) { x -= Right.x; y -= Right.y; z -= Right.z; return *this; }
};
struct vec4_t
{
    bfloat16_t x{}, y{}, z{}, w{};

    vec4_t() = default;
    template<typename T> operator T() const { return { x, y, z, w }; }
    operator bool() const { return !!(x.Value + y.Value + z.Value + w.Value); }
    vec4_t(vec2_t XY, vec2_t WH) : x(XY.x), y(XY.y), z(x + WH.x), w(y + WH.y) {};
    template<typename T = int> vec4_t(vec3_t XYZ, T W = 0) : x(XYZ.x), y(XYZ.y), z(XYZ.z), w(W) {}
    template<typename T, typename U, typename V> vec4_t(T X, U Y, V Z, T W) : x(X), y(Y), z(Z), w(W) {}
    template<typename T, typename U, typename V> vec4_t(T X, U Y, V Z, U W) : x(X), y(Y), z(Z), w(W) {}

    bool operator!=(const vec4_t &Right) const { return !operator==(Right); }
    bool operator==(const vec4_t &Right) const { return x == Right.x && y == Right.y && z == Right.w && z == Right.w; }
    bool operator<(const vec4_t &Right)  const { return (x < Right.x) && (y < Right.y) && (z < Right.z) && (w < Right.w); }
    bool operator>(const vec4_t &Right)  const { return (x > Right.x) && (y > Right.y) && (z > Right.z) && (w > Right.w); }
    bool operator<=(const vec4_t &Right) const { return (x <= Right.x) && (y <= Right.y) && (z <= Right.z) && (w <= Right.w); }
    bool operator>=(const vec4_t &Right) const { return (x >= Right.x) && (y >= Right.y) && (z >= Right.z) && (w >= Right.w); }

    friend vec4_t operator+(vec4_t Left, const vec4_t &Right) { Left += Right; return Left; }
    friend vec4_t operator-(vec4_t Left, const vec4_t &Right) { Left -= Right; return Left; }
    friend vec4_t operator*(vec4_t Left, const bfloat16_t &Right) { Left *= Right; return Left; }

    vec4_t &operator*=(const bfloat16_t &Right) { x *= Right; y *= Right; z *= Right; return *this; }
    vec4_t &operator+=(const vec4_t &Right) { x += Right.x; y += Right.y; z += Right.z; return *this; }
    vec4_t &operator-=(const vec4_t &Right) { x -= Right.x; y -= Right.y; z -= Right.z; return *this; }
};

struct rgb_t { uint8_t r, g, b; };
struct Color_t : rgb_t
{
    uint8_t a;

    Color_t() : rgb_t() { a = 0xFF; }
    Color_t(rgb_t RGB) : rgb_t(RGB) { a = 0xFF; };
    Color_t(uint8_t R, uint8_t G, uint8_t B, uint8_t A = 0xFF) : rgb_t{ R, G, B }, a(A) {}
    Color_t(float R, float G, float B, float A = 1.0f) { r = R * 0xFF; g = G * 0xFF; b = B * 0xFF; a = A * 0xFF; }
    Color_t(COLORREF RGBA) { r = RGBA & 0xFF; g = (RGBA >> 8) & 0xFF;  b = (RGBA >> 16) & 0xFF; a = (RGBA >> 24) & 0xFF; }

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

// Constants used everywhere.
#pragma region Constants

// NOTE(tcn): Windows gets confused if Alpha != NULL.
constexpr COLORREF Clearcolor{ 0x00FFFFFF };

#pragma endregion

// Subsystems that depend on the datatypes.
#include <Subsystems/Overlay/Overlay.hpp>
#include <Subsystems/Pluginloader/Pluginloader.hpp>

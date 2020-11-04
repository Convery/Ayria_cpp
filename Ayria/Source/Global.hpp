
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
    uint16_t Value{};
    using FP = union { uint32_t I; float F; };

    static constexpr bfloat16_t Round(float Input)
    {
        FP Value; Value.F = Input;

        // Special cases, min-max.
        if ((Value.I & 0xFF800000) == 0) return uint16_t(0);
        if ((Value.I & 0xFF800000) == 0x80000000) return uint16_t(0x8000);

        const auto LSB = (Value.I >> 16) & 1;
        return uint16_t((Value.I + 0x7FFF + LSB) >> 16);
    }

    constexpr bfloat16_t() = default;
    template<typename T> constexpr operator T() const { FP Output; Output.I = Value << 16; return T(Output.F); }
    template<typename T, typename = std::enable_if_t<std::is_integral_v<T> || std::is_floating_point_v<T>, T>>
    constexpr bfloat16_t(T Input)
    {
        if constexpr (std::is_same_v<T, uint16_t>) Value = Input;
        else *this = Round(float(Input));
    }

    constexpr bool operator!=(const bfloat16_t &Right) const { return !operator==(Right); }
    constexpr bool operator==(const bfloat16_t &Right) const { return Value == Right.Value; }
    template<typename T> constexpr bool operator<(const T &Right)  const { return float(*this) < float(Right); }
    template<typename T> constexpr bool operator>(const T &Right)  const { return float(*this) > float(Right); }
    template<typename T> constexpr bool operator<=(const T &Right) const { return float(*this) <= float(Right); }
    template<typename T> constexpr bool operator>=(const T &Right) const { return float(*this) >= float(Right); }

    template<typename T> constexpr friend bfloat16_t operator+(bfloat16_t Left, const T &Right) { Left += Right; return Left; }
    template<typename T> constexpr friend bfloat16_t operator-(bfloat16_t Left, const T &Right) { Left -= Right; return Left; }
    template<typename T> constexpr friend bfloat16_t operator*(bfloat16_t Left, const T &Right) { Left *= Right; return Left; }
    template<typename T> constexpr friend bfloat16_t operator/(bfloat16_t Left, const T &Right) { Left /= Right; return Left; }

    template<typename T> constexpr bfloat16_t &operator+=(const T &Right) { const float L = *this, R = Right; *this = (L + R); return *this; }
    template<typename T> constexpr bfloat16_t &operator-=(const T &Right) { const float L = *this, R = Right; *this = (L - R); return *this; }
    template<typename T> constexpr bfloat16_t &operator*=(const T &Right) { const float L = *this, R = Right; *this = (L * R); return *this; }
    template<typename T> constexpr bfloat16_t &operator/=(const T &Right) { const float L = *this, R = Right; *this = (L / R); return *this; }
};
struct vec2_t
{
    bfloat16_t x{}, y{};

    constexpr vec2_t() = default;
    constexpr operator bool() const { return !!(x.Value + y.Value); }
    template<typename T> constexpr operator T() const { return { x, y }; }
    template<typename T, typename U> constexpr vec2_t(T X, U Y) : x(X), y(Y) {}

    bool operator!=(const vec2_t &Right) const { return !operator==(Right); }
    bool operator==(const vec2_t &Right) const { return x == Right.x && y == Right.y; }
    bool operator<(const vec2_t &Right)  const { return (x < Right.x) || (y < Right.y); }
    bool operator>(const vec2_t &Right)  const { return (x > Right.x) || (y > Right.y); }
    bool operator<=(const vec2_t &Right) const { return (x <= Right.x) || (y <= Right.y); }
    bool operator>=(const vec2_t &Right) const { return (x >= Right.x) || (y >= Right.y); }

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

    constexpr vec3_t() = default;
    constexpr operator vec2_t() const { return { x, y }; }
    template<typename T> constexpr operator T() const { return { x, y, z }; }
    constexpr operator bool() const { return !!(x.Value + y.Value + z.Value); }
    template<typename T> constexpr vec3_t(T X, T Y, T Z) : x(X), y(Y), z(Z) {}
    template<typename T = int> constexpr vec3_t(vec2_t XY, T Z = 0) : x(XY.x), y(XY.y), z(Z) {}
    template<typename T, typename U, typename V> constexpr vec3_t(T X, U Y, V Z) : x(X), y(Y), z(Z) {}

    bool operator!=(const vec3_t &Right) const { return !operator==(Right); }
    bool operator==(const vec3_t &Right) const { return x == Right.x && y == Right.y && z == Right.z; }
    bool operator<(const vec3_t &Right)  const { return (x < Right.x) || (y < Right.y) || (z < Right.z); }
    bool operator>(const vec3_t &Right)  const { return (x > Right.x) || (y > Right.y) || (z > Right.z); }
    bool operator<=(const vec3_t &Right) const { return (x <= Right.x) || (y <= Right.y) || (z <= Right.z); }
    bool operator>=(const vec3_t &Right) const { return (x >= Right.x) || (y >= Right.y) || (z >= Right.z); }

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

    constexpr vec4_t() = default;
    template<typename T> constexpr operator T() const { return { x, y, z, w }; }
    constexpr operator bool() const { return !!(x.Value + y.Value + z.Value + w.Value); }
    template<typename T> constexpr vec4_t(T X, T Y, T Z, T W) : x(X), y(Y), z(Z), w(W) {}
    constexpr vec4_t(vec2_t XY, vec2_t WH) : x(XY.x), y(XY.y), z(x + WH.x), w(y + WH.y) {}
    template<typename T = int> constexpr vec4_t(vec3_t XYZ, T W = 0) : x(XYZ.x), y(XYZ.y), z(XYZ.z), w(W) {}
    template<typename T, typename U, typename V> constexpr vec4_t(T X, U Y, V Z, T W) : x(X), y(Y), z(Z), w(W) {}
    template<typename T, typename U, typename V> constexpr vec4_t(T X, U Y, V Z, U W) : x(X), y(Y), z(Z), w(W) {}

    bool operator!=(const vec4_t &Right) const { return !operator==(Right); }
    bool operator==(const vec4_t &Right) const { return x == Right.x && y == Right.y && z == Right.w && w == Right.w; }
    bool operator<(const vec4_t &Right)  const { return (x < Right.x) || (y < Right.y) || (z < Right.z) || (w < Right.w); }
    bool operator>(const vec4_t &Right)  const { return (x > Right.x) || (y > Right.y) || (z > Right.z) || (w > Right.w); }
    bool operator<=(const vec4_t &Right) const { return (x <= Right.x) || (y <= Right.y) || (z <= Right.z) || (w <= Right.w); }
    bool operator>=(const vec4_t &Right) const { return (x >= Right.x) || (y >= Right.y) || (z >= Right.z) || (w >= Right.w); }

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

    constexpr Color_t() : rgb_t() { a = 0xFF; }
    constexpr Color_t(rgb_t RGB) : rgb_t(RGB) { a = 0xFF; };
    constexpr Color_t(uint8_t R, uint8_t G, uint8_t B, uint8_t A = 0xFF) : rgb_t{ R, G, B }, a(A) {}
    constexpr Color_t(COLORREF RGBA) { r = RGBA & 0xFF; g = (RGBA >> 8) & 0xFF;  b = (RGBA >> 16) & 0xFF; a = (RGBA >> 24) & 0xFF; }

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

using AccountID_t = Ayriamodule_t::AccountID_t;
using Eventflags_t = union
{
    union
    {
        uint32_t Raw;
        uint32_t Any;
        struct
        {
            uint32_t
                onWindowchange : 1,
                onCharinput : 1,

                doBackspace : 1,
                doDelete : 1,
                doCancel : 1,
                doPaste : 1,
                doEnter : 1,
                doUndo : 1,
                doRedo : 1,
                doCopy : 1,
                doTab : 1,
                doCut : 1,

                Mousemove : 1,
                Mousedown : 1,
                Mouseup : 1,
                Keydown : 1,
                Keyup : 1,

                Mousemiddle : 1,
                Mouseright : 1,
                Mouseleft : 1,

                modShift : 1,
                modCtrl : 1,

                FLAG_MAX : 1;
        };
    };
};
using Account_t = struct
{
    AccountID_t ID;
    std::u8string Locale;
    std::u8string Username;
};

#pragma pack(pop)
#pragma endregion

// Constants used everywhere.
#pragma region Constants

// NOTE(tcn): Windows gets confused if Alpha != NULL.
constexpr COLORREF Clearcolor{ 0x00FFFFFF };

#pragma endregion

// C-exports, JSON in and JSON out.
namespace API
{
    using Functionhandler = std::string (__cdecl *)(const char *JSONString);

    void Registerhandler_Client(std::string_view Function, Functionhandler Handler);
    void Registerhandler_Social(std::string_view Function, Functionhandler Handler);
    void Registerhandler_Network(std::string_view Function, Functionhandler Handler);
    void Registerhandler_Matchmake(std::string_view Function, Functionhandler Handler);
    void Registerhandler_Fileshare(std::string_view Function, Functionhandler Handler);

    // FunctionID = FNV1_32("Service name"); ID 0 / Invalid = List all available.
    extern "C" EXPORT_ATTR const char *__cdecl API_Client(uint32_t FunctionID, const char *JSONString);
    extern "C" EXPORT_ATTR const char *__cdecl API_Social(uint32_t FunctionID, const char *JSONString);
    extern "C" EXPORT_ATTR const char *__cdecl API_Network(uint32_t FunctionID, const char *JSONString);
    extern "C" EXPORT_ATTR const char *__cdecl API_Matchmake(uint32_t FunctionID, const char *JSONString);
    extern "C" EXPORT_ATTR const char *__cdecl API_Fileshare(uint32_t FunctionID, const char *JSONString);
}

// Core systems.
#include <Backend/Backend.hpp>

// Subsystems that depend on the datatypes.
#include <Subsystems/Pluginloader/Pluginloader.hpp>
#include <Subsystems/Overlay/Rendering.hpp>
#include <Subsystems/Overlay/Overlay.hpp>
#include <Subsystems/Console/Console.hpp>

// Common functionality.
#include <Common/Social.hpp>
#include <Common/Fileshare.hpp>
#include <Common/Clientinfo.hpp>
#include <Common/Matchmaking.hpp>

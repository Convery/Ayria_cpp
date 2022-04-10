/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-11-07
    License: MIT

    NOTE:
    float16_t for smaller numbers in general calculations.
    bfloat16_t for large numbers that don't need accuracy.
*/

#pragma once
#include <intrin.h>
#include <cstdint>
#include <numbers>
#include <string>
#include <cmath>
#include <bit>

//
#pragma pack(push, 1)

// Generic datatype for dynamic arrays.
using Blob_t = std::basic_string<uint8_t>;
using Blob_view_t = std::basic_string_view<uint8_t>;

// Accurate until +/-2048. e.g. 2051.0f gets truncated to 2052.0f.
struct float16_t
{
    uint16_t Value{};

    // AVX provides intrinsics for doing this, but seems slower for a single conversion.
    static constexpr float toFloat(const uint16_t Input)
    {
        const uint32_t Words = uint32_t(Input) << 16;
        const uint32_t Sign = Words & 0x80000000U;
        const uint32_t Mantissa = Words + Words;

        // Denormal.
        if (Mantissa < 0x8000000U)
        {
            const float Denormalized = std::bit_cast<float>((Mantissa >> 17) | 0x3F000000U) - 0.5f;
            return std::bit_cast<float>(Sign | std::bit_cast<uint32_t>(Denormalized));
        }
        else
        {
            const float Scale = std::bit_cast<float>(0x7800000U);
            const float Normalized = std::bit_cast<float>((Mantissa >> 4) + 0x70000000U) * Scale;
            return std::bit_cast<float>(Sign | std::bit_cast<uint32_t>(Normalized));
        }
    }
    static constexpr float toFloat(const float16_t &Input)
    {
        return toFloat(Input.Value);
    }
    static constexpr uint16_t fromFloat(const float Input)
    {
        constexpr float zeroScale = std::bit_cast<float>(0x08800000U);
        constexpr float infScale = std::bit_cast<float>(0x77800000U);

        const uint32_t Words = std::bit_cast<uint32_t>(Input);
        const uint32_t Sign = Words & 0x80000000U;
        const uint32_t Mantissa = Words + Words;

        // Out of range.
        if (Mantissa > 0xFF000000U)
            return (Sign >> 16) | 0x7E00;

        // Don't care about sign of 0.
        const float ABS = Input > 0 ? Input : -Input;
        const float Normalized = ABS * (infScale * zeroScale);
        const uint32_t Bias = std::max(Mantissa & 0xFF000000U, 0x71000000U);
        const uint32_t Bits = std::bit_cast<uint32_t>(std::bit_cast<float>((Bias >> 1) + 0x07800000U) + Normalized);

        return (Sign >> 16) | (((Bits >> 13) & 0x00007C00U) + (Bits & 0x00000FFFU));
    }

    constexpr operator float() const { return toFloat(Value); }
    constexpr operator int32_t() const { return int32_t(toFloat(Value)); }

    constexpr float16_t() = default;
    constexpr float16_t(const float Input) : Value(fromFloat(Input)) {};
    explicit constexpr float16_t(const uint16_t Input) : Value(Input) {}
    template <std::integral T> constexpr float16_t(const T Input) : float16_t(float(Input)) {}

    constexpr bool operator<(const float16_t &Right)  const { return toFloat(Value) < toFloat(Right); }
    constexpr bool operator>(const float16_t &Right)  const { return toFloat(Value) > toFloat(Right); }
    constexpr bool operator<=(const float16_t &Right) const { return toFloat(Value) <= toFloat(Right); }
    constexpr bool operator>=(const float16_t &Right) const { return toFloat(Value) >= toFloat(Right); }
    constexpr bool operator!=(const float16_t &Right) const { return !operator==(Right); }
    constexpr bool operator==(const float16_t &Right) const
    {
        if (Value == Right.Value) return true;

        constexpr float Epsilon = 0.000977f;
        if (std::is_constant_evaluated())
        {
            const auto Temp = toFloat(Value) - toFloat(Right);
            return ((Temp >= 0) ? Temp : -Temp) < Epsilon;
        }
        else
            return std::fabs(toFloat(Value) - toFloat(Right)) < Epsilon;
    }

    constexpr float16_t &operator+=(const float16_t &Right) { *this = (toFloat(*this) + toFloat(Right)); return *this; }
    constexpr float16_t &operator-=(const float16_t &Right) { *this = (toFloat(*this) - toFloat(Right)); return *this; }
    constexpr float16_t &operator*=(const float16_t &Right) { *this = (toFloat(*this) * toFloat(Right)); return *this; }
    constexpr float16_t &operator/=(const float16_t &Right) { *this = (toFloat(*this) / toFloat(Right)); return *this; }

    friend constexpr float16_t operator+(const float16_t &Left, const float16_t &Right) { return toFloat(Left) + toFloat(Right); }
    friend constexpr float16_t operator-(const float16_t &Left, const float16_t &Right) { return toFloat(Left) - toFloat(Right); }
    friend constexpr float16_t operator*(const float16_t &Left, const float16_t &Right) { return toFloat(Left) * toFloat(Right); }
    friend constexpr float16_t operator/(const float16_t &Left, const float16_t &Right) { return toFloat(Left) / toFloat(Right); }
};

// Accurate until +/-256. e.g. 305.0f gets truncated to 304.0f.
struct bfloat16_t
{
    uint16_t Value{};

    // Currently prefers rounding to truncating, may change in the future.
    static constexpr uint16_t Truncate(const float Input)
    {
        // Quiet NaN
        if (Input != Input)
            return 0x7FC0;

        // Flush denormals to +0 or -0.
        if ((Input < 0 ? -Input : Input) < std::numeric_limits<float>::min())
            return (Input < 0) ? 0x8000U : 0;

        constexpr auto Offset = std::endian::native == std::endian::little;
        const auto Word = std::bit_cast<std::array<uint16_t, 2>>(Input);
        return Word[Offset];
    }
    static constexpr uint16_t Round(const float Input)
    {
        // Quiet NaN
        if (Input != Input)
            return 0x7FC1;

        // Flush denormals to +0 or -0.
        if ((Input < 0 ? -Input : Input) < std::numeric_limits<float>::min())
            return (Input < 0) ? 0x8000U : 0;

        // Constexpr does not like unions / normal casts.
        return static_cast<uint16_t>(uint32_t(std::bit_cast<uint32_t>(Input) + 0x00007FFFUL + ((std::bit_cast<uint32_t>(Input) >> 16) & 1)) >> 16);
    }

    // Fast conversion to IEEE 754
    static constexpr float toFloat(const uint16_t Input)
    {
        constexpr auto Offset = std::endian::native == std::endian::little;
        std::array<uint16_t, 2> Word{};
        Word[Offset] = Input;

        return std::bit_cast<float>(Word);
    }
    static constexpr float toFloat(const bfloat16_t &Input)
    {
        return toFloat(Input.Value);
    }

    constexpr operator float() const { return toFloat(Value); }
    constexpr operator int32_t() const { return int32_t(toFloat(Value)); }

    constexpr bfloat16_t() = default;
    constexpr bfloat16_t(const float Input) : Value(Round(Input)) {};
    explicit constexpr bfloat16_t(const uint16_t Input) : Value(Input) {}
    template <std::integral T> constexpr bfloat16_t(const T Input) : bfloat16_t(float(Input)) {}

    constexpr bool operator<(const bfloat16_t &Right)  const { return toFloat(Value) < toFloat(Right); }
    constexpr bool operator>(const bfloat16_t &Right)  const { return toFloat(Value) > toFloat(Right); }
    constexpr bool operator<=(const bfloat16_t &Right) const { return toFloat(Value) <= toFloat(Right); }
    constexpr bool operator>=(const bfloat16_t &Right) const { return toFloat(Value) >= toFloat(Right); }
    constexpr bool operator!=(const bfloat16_t &Right) const { return !operator==(Right); }
    constexpr bool operator==(const bfloat16_t &Right) const
    {
        if (Value == Right.Value) return true;

        constexpr float Epsilon = 0.000977f;
        if (std::is_constant_evaluated())
        {
            const auto Temp = toFloat(Value) - toFloat(Right);
            return ((Temp >= 0) ? Temp : -Temp) < Epsilon;
        }
        else
            return std::fabs(toFloat(Value) - toFloat(Right)) < Epsilon;
    }

    constexpr bfloat16_t &operator+=(const bfloat16_t &Right) { *this = (toFloat(*this) + toFloat(Right)); return *this; }
    constexpr bfloat16_t &operator-=(const bfloat16_t &Right) { *this = (toFloat(*this) - toFloat(Right)); return *this; }
    constexpr bfloat16_t &operator*=(const bfloat16_t &Right) { *this = (toFloat(*this) * toFloat(Right)); return *this; }
    constexpr bfloat16_t &operator/=(const bfloat16_t &Right) { *this = (toFloat(*this) / toFloat(Right)); return *this; }

    friend constexpr bfloat16_t operator+(const bfloat16_t &Left, const bfloat16_t &Right) { return toFloat(Left) + toFloat(Right); }
    friend constexpr bfloat16_t operator-(const bfloat16_t &Left, const bfloat16_t &Right) { return toFloat(Left) - toFloat(Right); }
    friend constexpr bfloat16_t operator*(const bfloat16_t &Left, const bfloat16_t &Right) { return toFloat(Left) * toFloat(Right); }
    friend constexpr bfloat16_t operator/(const bfloat16_t &Left, const bfloat16_t &Right) { return toFloat(Left) / toFloat(Right); }
};

template <typename T>
struct vec2_t
{
    T x{}, y{};

    constexpr vec2_t() = default;
    constexpr operator bool() const { return !!(x + y); }
    constexpr vec2_t(const vec2_t &Other) { x = Other.x; y = Other.y; }
    template<typename U> constexpr operator U() const { return { x, y }; }
    template<typename A, typename B> constexpr vec2_t(A X, B Y) : x(X), y(Y) {}

    constexpr bool operator!=(const vec2_t &Right) const { return !operator==(Right); }
    constexpr bool operator==(const vec2_t &Right) const { return x == Right.x && y == Right.y; }
    constexpr bool operator<(const vec2_t &Right)  const { return (x < Right.x) || (y < Right.y); }
    constexpr bool operator>(const vec2_t &Right)  const { return (x > Right.x) || (y > Right.y); }
    constexpr bool operator<=(const vec2_t &Right) const { return (x <= Right.x) || (y <= Right.y); }
    constexpr bool operator>=(const vec2_t &Right) const { return (x >= Right.x) || (y >= Right.y); }

    constexpr vec2_t &operator*=(const T &Right) { x *= Right; y *= Right; return *this; }
    constexpr vec2_t &operator+=(const vec2_t &Right) { x += Right.x; y += Right.y; return *this; }
    constexpr vec2_t &operator-=(const vec2_t &Right) { x -= Right.x; y -= Right.y; return *this; }

    constexpr friend vec2_t operator*(vec2_t Left, const T &Right) { Left *= Right; return Left; }
    constexpr friend vec2_t operator+(vec2_t Left, const vec2_t &Right) { Left += Right; return Left; }
    constexpr friend vec2_t operator-(vec2_t Left, const vec2_t &Right) { Left -= Right; return Left; }
};
using vec2f = vec2_t<float16_t>;
using vec2u = vec2_t<uint16_t>;
using vec2i = vec2_t<int16_t>;

template <typename T>
struct vec3_t
{
    T x{}, y{}, z{};

    constexpr vec3_t() = default;
    constexpr operator bool() const { return !!(x + y + z); }
    template<typename U> constexpr operator U() const { return { x, y, z }; }
    constexpr vec3_t(const vec3_t &Other) { x = Other.x; y = Other.y; z = Other.z; }
    template<typename A, typename B, typename C> constexpr vec3_t(A X, B Y, C Z) : x(X), y(Y), z(Z) {}

    constexpr bool operator!=(const vec3_t &Right) const { return !operator==(Right); }
    constexpr bool operator==(const vec3_t &Right) const { return x == Right.x && y == Right.y && z == Right.z; }
    constexpr bool operator<(const vec3_t &Right)  const { return (x < Right.x) || (y < Right.y) || (z < Right.z); }
    constexpr bool operator>(const vec3_t &Right)  const { return (x > Right.x) || (y > Right.y) || (z > Right.z); }
    constexpr bool operator<=(const vec3_t &Right) const { return (x <= Right.x) || (y <= Right.y) || (z <= Right.z); }
    constexpr bool operator>=(const vec3_t &Right) const { return (x >= Right.x) || (y >= Right.y) || (z >= Right.z); }

    constexpr vec3_t &operator*=(const T &Right) { x *= Right; y *= Right; z *= Right; return *this; }
    constexpr vec3_t &operator+=(const vec3_t &Right) { x += Right.x; y += Right.y; z += Right.z; return *this; }
    constexpr vec3_t &operator-=(const vec3_t &Right) { x -= Right.x; y -= Right.y; z -= Right.z; return *this; }

    constexpr friend vec3_t operator*(vec3_t Left, const T &Right) { Left *= Right; return Left; }
    constexpr friend vec3_t operator+(vec3_t Left, const vec3_t &Right) { Left += Right; return Left; }
    constexpr friend vec3_t operator-(vec3_t Left, const vec3_t &Right) { Left -= Right; return Left; }
};
using vec3f = vec3_t<float16_t>;
using vec3u = vec3_t<uint16_t>;
using vec3i = vec3_t<int16_t>;

template <typename T>
struct vec4_t
{
    union
    {
        #pragma warning (suppress: 4201)
        struct { vec2_t<T> ab, cd; };
        #pragma warning (suppress: 4201)
        struct { T x, y, z, w; };
    };

    constexpr vec4_t() { x = y = z = w = 0; }
    constexpr vec4_t(vec2_t<T> X, vec2_t<T> Y) : ab(X), cd(Y) {}
    template<typename U> constexpr operator U() const { return { x, y, z, w }; }
    constexpr vec4_t(const vec4_t &Other) { x = Other.x; y = Other.y; z = Other.z; w = Other.w; }
    template<typename A, typename B, typename C, typename D> constexpr vec4_t(A X, B Y, C Z, D W) : x(X), y(Y), z(Z), w(W) {}

    constexpr operator bool() const { return !!(x + y + z + w); }
    constexpr bool operator!=(const vec4_t &Right) const { return !operator==(Right); }
    constexpr bool operator==(const vec4_t &Right) const { return ab == Right.ab && cd == Right.cd; }
    constexpr bool operator<(const vec4_t &Right)  const { return (ab < Right.ab) || (cd < Right.cd); }
    constexpr bool operator>(const vec4_t &Right)  const { return (ab > Right.ab) || (cd > Right.cd); }
    constexpr bool operator<=(const vec4_t &Right) const { return (ab <= Right.ab) || (cd <= Right.cd); }
    constexpr bool operator>=(const vec4_t &Right) const { return (ab >= Right.ab) || (cd >= Right.cd); }

    constexpr vec4_t &operator*=(const T &Right) { ab *= Right; cd *= Right; return *this; }
    constexpr vec4_t &operator+=(const vec4_t &Right) { ab += Right.ab; cd += Right.cd; return *this; }
    constexpr vec4_t &operator-=(const vec4_t &Right) { ab -= Right.ab; cd -= Right.cd; return *this; }

    constexpr friend vec4_t operator*(vec4_t Left, const T &Right) { Left *= Right; return Left; }
    constexpr friend vec4_t operator+(vec4_t Left, const vec4_t &Right) { Left += Right; return Left; }
    constexpr friend vec4_t operator-(vec4_t Left, const vec4_t &Right) { Left -= Right; return Left; }
};
using vec4f = vec4_t<float16_t>;
using vec4u = vec4_t<uint16_t>;
using vec4i = vec4_t<int16_t>;

#pragma pack(pop)

/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-11-07
    License: MIT
*/

#pragma once
#include <cstdint>
#include <numeric>
#include <cmath>
#include <bit>

//
#pragma pack(push, 1)

// See ML-frameworks like Tensorflow for optimisation ideas of bfloat16.
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
        const auto Integer = std::bit_cast<uint32_t>(Input);

        // Compiletime path.
        if (std::is_constant_evaluated())
        {
            // Quiet NaN
            if (Input != Input)
                return 0x7FC0;

            // Flush denormals to +0 or -0.
            if ((Input < 0 ? -Input : Input) < std::numeric_limits<float>::min())
                return (Input < 0) ? 0x8000U : 0;

            // Constexpr does not like unions / normal casts.
            return static_cast<uint16_t>(uint32_t(Integer + 0x00007FFFUL + ((Integer >> 16) & 1)) >> 16);
        }

        // Runtime path.
        else
        {
            switch (std::fpclassify(Input))
            {
                // Quiet NaN
                case FP_NAN:
                   return (Integer >> 16) | (1 << 6);

                // Flush denormals to +0 or -0.
                case FP_SUBNORMAL:
                case FP_ZERO:
                    return (Integer >> 16) & 0x8000;

                case FP_INFINITE:
                    return (Integer >> 16);

                case FP_NORMAL:
                    return static_cast<uint16_t>(uint32_t(Integer + 0x00007FFFUL + ((Integer >> 16) & 1)) >> 16);

                // Should never happen, return NaN.
                default: return 0x7FC0;
            }
        }
    }

    constexpr bfloat16_t() = default;
    constexpr bfloat16_t(const uint16_t Input) : Value(Input) {};
    constexpr bfloat16_t(const bfloat16_t &Input) : Value(Input.Value) {};
    template <std::floating_point T> constexpr bfloat16_t(const T Input) : Value(Round(static_cast<float>(Input))) {}
    template <std::signed_integral T> constexpr bfloat16_t(const T Input) : Value(Round(static_cast<float>(Input))) {}

    // Fast conversion to IEEE 754
    constexpr float toFloat() const
    {
        constexpr auto Offset = std::endian::native == std::endian::little;
        std::array<uint16_t, 2> Word{};
        Word[Offset] = Value;

        return std::bit_cast<float>(Word);
    }
    static constexpr float toFloat(const bfloat16_t &Input)
    {
        return Input.toFloat();
    }

    // Provides higher presicion for some common constants.
    template <std::floating_point T> constexpr operator T() const
    {
        if (Value == 0x4049U) [[unlikely]] return T(std::numbers::pi);
        if (Value == 0x3EABU) [[unlikely]] return T(1.0 / 3.0);

        return toFloat();
    }
    template <std::signed_integral T> constexpr operator T() const
    {
        return T(float(*this));
    }

    // Negate and check against null.
    constexpr bfloat16_t operator-(const bfloat16_t &Input) const
    {
        return bfloat16_t(uint16_t(Input.Value ^ 0x8000));
    }
    constexpr operator bool() const
    {
        return !!(Value & 0x7FFF);
    }

    constexpr bool operator<(const bfloat16_t &Right)  const { return toFloat(Value) <  toFloat(Right); }
    constexpr bool operator>(const bfloat16_t &Right)  const { return toFloat(Value) >  toFloat(Right); }
    constexpr bool operator<=(const bfloat16_t &Right) const { return toFloat(Value) <= toFloat(Right); }
    constexpr bool operator>=(const bfloat16_t &Right) const { return toFloat(Value) >= toFloat(Right); }
    constexpr bool operator!=(const bfloat16_t &Right) const { return !operator==(Right); }
    constexpr bool operator==(const bfloat16_t &Right) const
    {
        if (Value == Right.Value) return true;

        constexpr float Epsilon = 0.00781250f;
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
using vec2f = vec2_t<bfloat16_t>;
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
using vec3f = vec3_t<bfloat16_t>;
using vec3u = vec3_t<uint16_t>;
using vec3i = vec3_t<int16_t>;

template <typename T>
struct vec4_t
{
    union
    {
        struct { vec2_t<T> ab, cd; };
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
using vec4f = vec4_t<bfloat16_t>;
using vec4u = vec4_t<uint16_t>;
using vec4i = vec4_t<int16_t>;

#pragma pack(pop)

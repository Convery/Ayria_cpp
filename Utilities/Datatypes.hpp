/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-11-07
    License: MIT
*/

#pragma once
#include <cstdint>
#include <type_traits>

//
#pragma pack(push, 1)

// See ML-frameworks like Tensorflow for optimisation ideas of bfloat16.
struct bfloat16_t
{
    uint16_t Value{};
    using FP = union { uint32_t I; float F; };

    static constexpr bfloat16_t Round(float Input)
    {
        const FP Value{ .F = Input };

        // Special cases, min-max.
        if ((Value.I & 0xFF800000) == 0) return static_cast<uint16_t>(0);
        if ((Value.I & 0xFF800000) == 0x80000000) return static_cast<uint16_t>(0x8000);

        const auto LSB = (Value.I >> 16) & 1;
        return static_cast<uint16_t>((Value.I + 0x7FFF + LSB) >> 16);
    }

    constexpr bfloat16_t() = default;
    template<typename T> constexpr operator T() const { return T((FP{ .I = static_cast<uint32_t>(Value << 16) }).F); }
    template<typename T, typename = std::enable_if_t<std::is_integral_v<T> || std::is_floating_point_v<T>, T>>
    constexpr bfloat16_t(T Input)
    {
        if constexpr (std::is_same_v<T, uint16_t>) Value = Input;
        else *this = Round(static_cast<float>(Input));
    }

    constexpr bool operator!=(const bfloat16_t &Right) const { return !operator==(Right); }
    constexpr bool operator<(const bfloat16_t &Right)  const { return Value < Right.Value; }
    constexpr bool operator>(const bfloat16_t &Right)  const { return Value > Right.Value; }
    constexpr bool operator<=(const bfloat16_t &Right) const { return Value <= Right.Value; }
    constexpr bool operator>=(const bfloat16_t &Right) const { return Value >= Right.Value; }
    constexpr bool operator==(const bfloat16_t &Right) const { return Value == Right.Value; }

    constexpr friend bfloat16_t operator+(bfloat16_t Left, const bfloat16_t &Right) { Left += Right; return Left; }
    constexpr friend bfloat16_t operator-(bfloat16_t Left, const bfloat16_t &Right) { Left -= Right; return Left; }
    constexpr friend bfloat16_t operator*(bfloat16_t Left, const bfloat16_t &Right) { Left *= Right; return Left; }
    constexpr friend bfloat16_t operator/(bfloat16_t Left, const bfloat16_t &Right) { Left /= Right; return Left; }

    constexpr bfloat16_t &operator+=(const bfloat16_t &Right) { *this = (float(*this) + float(Right)); return *this; }
    constexpr bfloat16_t &operator-=(const bfloat16_t &Right) { *this = (float(*this) - float(Right)); return *this; }
    constexpr bfloat16_t &operator*=(const bfloat16_t &Right) { *this = (float(*this) * float(Right)); return *this; }
    constexpr bfloat16_t &operator/=(const bfloat16_t &Right) { *this = (float(*this) / float(Right)); return *this; }
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

    vec2_t &operator*=(const T &Right) { x *= Right; y *= Right; return *this; }
    vec2_t &operator+=(const vec2_t &Right) { x += Right.x; y += Right.y; return *this; }
    vec2_t &operator-=(const vec2_t &Right) { x -= Right.x; y -= Right.y; return *this; }

    friend vec2_t operator*(vec2_t Left, const T &Right) { Left *= Right; return Left; }
    friend vec2_t operator+(vec2_t Left, const vec2_t &Right) { Left += Right; return Left; }
    friend vec2_t operator-(vec2_t Left, const vec2_t &Right) { Left -= Right; return Left; }
};
using vec2f = vec2_t<bfloat16_t>;
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

    vec3_t &operator*=(const T &Right) { x *= Right; y *= Right; z *= Right; return *this; }
    vec3_t &operator+=(const vec3_t &Right) { x += Right.x; y += Right.y; z += Right.z; return *this; }
    vec3_t &operator-=(const vec3_t &Right) { x -= Right.x; y -= Right.y; z -= Right.z; return *this; }

    friend vec3_t operator*(vec3_t Left, const T &Right) { Left *= Right; return Left; }
    friend vec3_t operator+(vec3_t Left, const vec3_t &Right) { Left += Right; return Left; }
    friend vec3_t operator-(vec3_t Left, const vec3_t &Right) { Left -= Right; return Left; }
};
using vec3f = vec3_t<bfloat16_t>;
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

    vec4_t &operator*=(const T &Right) { ab *= Right; cd *= Right; return *this; }
    vec4_t &operator+=(const vec4_t &Right) { ab += Right.ab; cd += Right.cd; return *this; }
    vec4_t &operator-=(const vec4_t &Right) { ab -= Right.ab; cd -= Right.cd; return *this; }

    friend vec4_t operator*(vec4_t Left, const T &Right) { Left *= Right; return Left; }
    friend vec4_t operator+(vec4_t Left, const vec4_t &Right) { Left += Right; return Left; }
    friend vec4_t operator-(vec4_t Left, const vec4_t &Right) { Left -= Right; return Left; }
};
using vec4f = vec4_t<bfloat16_t>;
using vec4i = vec4_t<int16_t>;

#pragma pack(pop)

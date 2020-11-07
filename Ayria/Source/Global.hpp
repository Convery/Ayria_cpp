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
// NOTE(tcn): Windows gets confused if Alpha != NULL.
constexpr COLORREF Clearcolor{ 0x00FFFFFF };

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

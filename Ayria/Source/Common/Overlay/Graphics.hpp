/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-07-06
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>
#include "Common.hpp"
#include "Drawing.hpp"

namespace Graphics
{
    using Eventflags_t = union
    {
        uint32_t Raw;
        uint32_t
          Mousemove : 1,
          Mousedown : 1,
          Mouseup : 1,
          Mouseleft : 1,
          Mouseright : 1,
          modShift : 1,
          modCtrl : 1,
          Keydown : 1,
          Keyup : 1,
          doCut : 1,
          doCopy : 1,
          doPaste : 1,
          doCancel : 1,
          doUndo : 1,
          doRedo : 1,
          doBackspace : 1,
          doDelete : 1,
          doEnter : 1,
          doTab : 1,
          onCharinput : 1,
          onWindowchange : 1;
    };

    struct Element_t;
    struct Event_t
    {
        Eventflags_t Flags;
        union
        {
            POINT Point;
            wchar_t Letter;
            uint32_t Keycode;
        } Data;
    };
    struct Surface_t
    {
        HDC Devicecontext{};
        HWND Windowhandle{};
        POINT Position{}, Size{};
        std::unordered_map<std::string, Element_t *> Elements{};
    };
    struct Element_t
    {
        Eventflags_t Wantedevents{};
        POINT Position{}, Size{};
        HDC Devicecontext{};
        HBITMAP Mask{};

        Element_t() = default;
        Element_t(Surface_t *Parent)
        {
            Devicecontext = CreateCompatibleDC(Parent->Devicecontext);
            const auto Bitmap = CreateCompatibleBitmap(Devicecontext, Size.x, Size.y);
            SelectObject(Devicecontext, ::Fonts::getDefaultfont());
            DeleteObject(SelectObject(Devicecontext, Bitmap));
        }

        // Update the internal state.
        virtual void onRender(Surface_t *Parent)
        {
            if (Devicecontext->unused == 0) [[unlikely]]
            {
                Devicecontext = CreateCompatibleDC(Parent->Devicecontext);
                const auto Bitmap = CreateCompatibleBitmap(Devicecontext, Size.x, Size.y);
                DeleteObject(SelectObject(Devicecontext, Bitmap));
            }
        }
        virtual void onEvent([[maybe_unused]] Surface_t *Parent, [[maybe_unused]] Event_t Event) {}
        virtual void onFrame([[maybe_unused]] Surface_t *Parent, [[maybe_unused]] float Deltatime) {}
    };


    void doFrame();
    Surface_t *Createsurface(std::string_view Identifier);
    constexpr COLORREF toColour(uint8_t R, uint8_t G, uint8_t B, uint8_t A = 0xFF)
    {
        return R | (G << 8) | (B << 16) | (A << 24);
    }
}

/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-07-06
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>
#include "Drawing.hpp"

namespace Graphics
{
    struct Surface_t;
    struct Event_t
    {
        union
        {
            uint32_t Raw;
            uint32_t
                    Mousemove : 1,
                    Mousedown : 1,
                    Mouseup : 1,
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
        } Flags;
        union
        {
            POINT Point;
            uint32_t Keycode;
            char Letter;
        } Data;
    };
    struct Element_t
    {
        uint32_t Wantedevents;
        POINT Position, Size;
        HDC Devicecontext;
        HBITMAP Mask;

        // Update the internal state.
        virtual void onRender(Surface_t *Parent) {}
        virtual void onEvent(Surface_t *Parent, Event_t Event) {}
        virtual void onFrame(Surface_t *Parent, float Deltatime) {}
    };
    struct Surface_t
    {
        bool isVisible;
        HDC Devicecontext;
        HWND Windowhandle;
        POINT Mouseposition;
        POINT Position, Size;
        std::vector<Element_t> Elements;
    };

    void doFrame();
    Surface_t *Createsurface(std::string_view Identifier);
    constexpr COLORREF toColor(uint8_t R, uint8_t G, uint8_t B, uint8_t A)
    {
        return R | (G << 8) | (B << 16) | (A << 24);
    }
}

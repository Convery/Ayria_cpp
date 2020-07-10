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
    struct Element_t
    {
        Eventflags_t Wantedevents{};
        POINT Position{}, Size{};
        HDC Devicecontext{};
        HBITMAP Mask{};

        // Only set if root/surface element.
        HWND Windowhandle{};

        // String identifier available while in beta for debugging.
        std::unordered_map<std::string, Element_t *> Children{};

        Element_t() = default;
        explicit Element_t(Element_t *Parent)
        {
            Devicecontext = CreateCompatibleDC(Parent->Devicecontext);
            const auto Bitmap = CreateCompatibleBitmap(Devicecontext, Size.x, Size.y);
            SelectObject(Devicecontext, Fonts::getDefault());
            DeleteObject(SelectObject(Devicecontext, Bitmap));
        }

        // Default implementation for easy inheritance.
        virtual void onRender(Element_t *Parent)
        {
            if (Devicecontext->unused == 0) [[unlikely]]
            {
                Devicecontext = CreateCompatibleDC(Parent->Devicecontext);
                const auto Bitmap = CreateCompatibleBitmap(Devicecontext, Size.x, Size.y);
                DeleteObject(SelectObject(Devicecontext, Bitmap));
            }

            for (auto &[ID, Element] : Children)
            {
                Element->onRender(this);

                // Transparency through masking.
                if (Element->Mask)
                {
                    const auto Device = CreateCompatibleDC(Element->Devicecontext);
                    SelectObject(Device, Element->Mask);

                    BitBlt(Devicecontext, Element->Position.x, Element->Position.y, Element->Size.x, Element->Size.y, Device, 0, 0, SRCAND);
                    BitBlt(Devicecontext, Element->Position.x, Element->Position.y, Element->Size.x, Element->Size.y, Element->Devicecontext, 0, 0, SRCPAINT);

                    DeleteDC(Device);
                }
                else
                {
                    BitBlt(Devicecontext, Element->Position.x, Element->Position.y, Element->Size.x, Element->Size.y, Element->Devicecontext, 0, 0, SRCCOPY);
                }
            }
        }
        virtual void onEvent([[maybe_unused]] Element_t *Parent, Event_t Event)
        {
            for (auto &[ID, Element] : Children)
                if (Element->Wantedevents.Raw & Event.Flags.Raw)
                    Element->onEvent(this, Event);
        }
        virtual void onFrame([[maybe_unused]] Element_t *Parent, float Deltatime)
        {
            for (auto &[ID, Element] : Children)
                Element->onFrame(this, Deltatime);
        }
    };

    // Element associated with a window, root.
    using Surface_t = Element_t;

    // Create a new window and update them.
    Surface_t *Createsurface(std::string_view Identifier, Surface_t *Original = nullptr);
    void Processsurfaces();
}

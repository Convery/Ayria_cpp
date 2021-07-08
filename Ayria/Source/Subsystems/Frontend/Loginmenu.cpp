/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-06-06
    License: MIT
*/

#include <Stdinclude.hpp>
#include <Global.hpp>

namespace Background
{
    static Elementinfo_t Elementinfo{};

    static void __cdecl onPaint(class Overlay_t *, const Graphics::Renderer_t &Renderer)
    {
        const Color_t Default(0xFF, 38, 35);
        #define Polyw(...) Renderer.Polygon( __VA_ARGS__, 1)->Render({}, Color_t(0xFF, 0xFF, 0xFF));
        #define Polyb(...) Renderer.Polygon( __VA_ARGS__, 1)->Render({}, Color_t(0x10, 0x10, 0x10));

        Renderer.Rectangle({}, Elementinfo.Size)->Render({}, Color_t(0xFF, 38, 35));
        Assets::JNZBG->Bitblt(Renderer.Devicecontext, { {}, Elementinfo.Size });

        const auto Filebuffer = FS::Readfile<char>("C:\\Polytest");
        if (Filebuffer.empty()) return;

        for (const auto &Line : lz::lines(Filebuffer))
        {
            static const std::regex Regex("\\{(\\d+),(\\d+)\\}");
            auto It = std::sregex_iterator(Line.cbegin(), Line.cend(), Regex);
            const auto Size = std::distance(It, std::sregex_iterator());

            if (Size == 0) continue;

            std::vector<vec2i> Points;
            Points.reserve(Size / 2);

            for (ptrdiff_t i = 0; i < Size; ++i)
            {
                auto Temp = (*It++).str();
                int X{}, Y{};

                std::sscanf(Temp.c_str(), "{%u,%u}", &X, &Y);
                Points.emplace_back(X, Y);
            }

            Renderer.Polygon(Points, 1)->Render({}, Color_t(0x00, 0xFF, 0x00));
        }
    }
    static void __cdecl onEvent(class Overlay_t *Parent, Eventflags_t Eventtype, const std::variant<uint32_t, vec2i> &Eventdata)
    {
        if (Eventtype.onWindowchange) [[likely]]
        {
            assert(std::holds_alternative<vec2i>(Eventdata));
            const auto Temp = std::get<vec2i>(Eventdata);
            if (Temp != Elementinfo.Size)
            {
                Elementinfo.Size = Temp;
                Parent->Invalidateelements();
            }
        }
    }
}




class Loginmenu_t : public Overlay_t
{

    protected:
    uint64_t Lasttime{};
    virtual void onTick(uint32_t DeltatimeMS)
    {
        const auto Currenttime = GetTickCount64();
        if (Currenttime - Lasttime > 500)
        {
            Invalidatescreen({}, Windowsize);
        }


        // We don't need to update the elements if invisible.
        if (!isVisible) [[likely]] return;


    }

    public:
    Loginmenu_t() : Overlay_t({}, {})
    {
        Background::Elementinfo.Eventmask.onWindowchange = true;
        Background::Elementinfo.onEvent = Background::onEvent;
        Background::Elementinfo.onPaint = Background::onPaint;
        Insertelement(&Background::Elementinfo);

        Invalidatescreen({}, Windowsize);

        RECT Workarea{};
        SystemParametersInfoA(SPI_GETWORKAREA, NULL, &Workarea, NULL);

        const vec2i Wantedsize{ 700, 986 };
        const vec2i Wantedposition{ (Workarea.right - Workarea.left - Wantedsize.x) / 2 ,
                                    (Workarea.bottom - Workarea.top - Wantedsize.y) / 2 };

        SetWindowPos(Windowhandle, NULL, Wantedposition.x, Wantedposition.y, Wantedsize.x, Wantedsize.y, SWP_ASYNCWINDOWPOS);
        SetFocus(Windowhandle);
    }
};

class Overlay_t *Createloginmenu()
{
    return new Loginmenu_t();
}

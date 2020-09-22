/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-09-21
    License: MIT
*/

#include <Stdinclude.hpp>
#include <Global.hpp>

// RTTI restoration of the pen.
std::unordered_map<int, HPEN, decltype(FNV::Hash), decltype(FNV::Equal)> Pens{};
std::unordered_map<COLORREF, HBRUSH> Brushes{};
struct Pen_t
{
    HGDIOBJ Previous;
    HDC DC;

    explicit Pen_t(HDC Devicecontext, int Linewidth, COLORREF Color)
    {
        if (!Pens.contains(Linewidth)) [[unlikely]]
            Pens[Linewidth] = CreatePen(PS_SOLID, Linewidth, Color);

        auto &Pen = Pens[Linewidth];
        Previous = SelectObject(Devicecontext, Pen);
        SetDCPenColor(Devicecontext, Color);
        DC = Devicecontext;
    }
    ~Pen_t()
    {
        SelectObject(DC, Previous);
    }
};
struct Brush_t
{
    HGDIOBJ Previous;
    HDC DC;

    explicit Brush_t(HDC Devicecontext, COLORREF Color)
    {
        if (!Brushes.contains(Color)) [[unlikely]]
            Brushes[Color] = CreateSolidBrush(Color);

        auto &Brush = Brushes[Color];
        Previous = SelectObject(Devicecontext, Brush);
        DC = Devicecontext;
    }
    ~Brush_t()
    {
        SelectObject(DC, Previous);
    }
};


namespace gInternal
{
    void Arc_t::Solid(COLORREF Color)
    {
        const Pen_t Pen(Devicecontext, 1, Color);
        const Brush_t Brush(Devicecontext, Color);

        SetArcDirection(Devicecontext, AD_COUNTERCLOCKWISE);
        Pie(Devicecontext, Boundingbox.x, Boundingbox.y, Boundingbox.z, Boundingbox.w,
            Focalpoint0.x, Focalpoint0.y, Focalpoint1.x, Focalpoint1.y);
    }
    void Arc_t::Solid(Texture2D Texture) { assert(false); }
    void Arc_t::Outline(uint8_t Linewidth, COLORREF Color)
    {
        const Pen_t Pen(Devicecontext, Linewidth, Color);
        const Brush_t Brush(Devicecontext, OPAQUE);

        SetArcDirection(Devicecontext, AD_COUNTERCLOCKWISE);
        Arc(Devicecontext, Boundingbox.x, Boundingbox.y, Boundingbox.z, Boundingbox.w,
            Focalpoint0.x, Focalpoint0.y, Focalpoint1.x, Focalpoint1.y);
    }
    void Arc_t::Filled(uint8_t Linewidth, COLORREF Outline, COLORREF Background)
    {
        const Pen_t Pen(Devicecontext, Linewidth, Outline);
        const Brush_t Brush(Devicecontext, Background);

        SetArcDirection(Devicecontext, AD_COUNTERCLOCKWISE);
        Pie(Devicecontext, Boundingbox.x, Boundingbox.y, Boundingbox.z, Boundingbox.w,
            Focalpoint0.x, Focalpoint0.y, Focalpoint1.x, Focalpoint1.y);
    }
    void Arc_t::Filled(uint8_t Linewidth, COLORREF Outline, Texture2D Background) { assert(false); }

    void Ellipse_t::Solid(COLORREF Color)
    {
        const Pen_t Pen(Devicecontext, 1, Color);
        const Brush_t Brush(Devicecontext, Color);
        Ellipse(Devicecontext, Dimensions.x, Dimensions.y, Dimensions.z, Dimensions.w);
    }
    void Ellipse_t::Solid(Texture2D Texture) { assert(false); }
    void Ellipse_t::Outline(uint8_t Linewidth, COLORREF Color)
    {
        const Pen_t Pen(Devicecontext, Linewidth, Color);
        const Brush_t Brush(Devicecontext, OPAQUE);

        Ellipse(Devicecontext, Dimensions.x, Dimensions.y, Dimensions.z, Dimensions.w);
    }
    void Ellipse_t::Filled(uint8_t Linewidth, COLORREF Outline, COLORREF Background)
    {
        const Pen_t Pen(Devicecontext, Linewidth, Outline);
        const Brush_t Brush(Devicecontext, Background);

        Ellipse(Devicecontext, Dimensions.x, Dimensions.y, Dimensions.z, Dimensions.w);
    }
    void Ellipse_t::Filled(uint8_t Linewidth, COLORREF Outline, Texture2D Background) { assert(false); }

    void Mesh_t::Solid()
    {
        GradientFill(Devicecontext, Vertices.get(), Vertexcount, Indicies.get(), Indexcount, GRADIENT_FILL_TRIANGLE);
    }
    void Mesh_t::Filled(uint8_t Linewidth)
    {
        Solid();
        Outline(Linewidth);
    }
    void Mesh_t::Outline(uint8_t Linewidth)
    {
        TRIVERTEX Lastvertex = Vertices[0];
        std::vector<vec2_t> Points;
        Points.reserve(Vertexcount);

        // Find solid lines.
        for (uint32_t i = 0; i < Vertexcount; ++i)
        {
            // Compare the color.
            if (0 == std::memcmp(&Vertices[i].Red, &Lastvertex.Red, sizeof(COLOR16) * 4))
            {
                Points.push_back({ int16_t(Vertices[i].x), int16_t(Vertices[i].y) });
            }
            else
            {
                const auto C = Color_t(Vertices[i].Red, Vertices[i].Green, Vertices[i].Blue, Vertices[i].Alpha);
                Path_t(Devicecontext, Points).Outline(Linewidth, C);

                Points.clear();
                Lastvertex = Vertices[i];
                Points.push_back({ int16_t(Vertices[i].x), int16_t(Vertices[i].y) });
            }
        }

        if (Points.size())
        {
            const auto C = Color_t(Lastvertex.Red, Lastvertex.Green, Lastvertex.Blue, Lastvertex.Alpha);
            Path_t(Devicecontext, Points).Outline(Linewidth, C);
        }
    }

    void Path_t::Solid(COLORREF Color)
    {
        const Pen_t Pen(Devicecontext, 1, Color);
        const Brush_t Brush(Devicecontext, Color);
        Polygon(Devicecontext, Points.get(), Pointcount);
    }
    void Path_t::Solid(Texture2D Texture) { assert(false); }
    void Path_t::Outline(uint8_t Linewidth, COLORREF Color)
    {
        const Pen_t Pen(Devicecontext, Linewidth, Color);
        const Brush_t Brush(Devicecontext, OPAQUE);

        Polyline(Devicecontext, Points.get(), Pointcount);
    }
    void Path_t::Filled(uint8_t Linewidth, COLORREF Outline, COLORREF Background)
    {
        const Pen_t Pen(Devicecontext, Linewidth, Outline);
        const Brush_t Brush(Devicecontext, Background);

        Polygon(Devicecontext, Points.get(), Pointcount);
    }
    void Path_t::Filled(uint8_t Linewidth, COLORREF Outline, Texture2D Background) { assert(false); }

    void Quad_t::Solid(COLORREF Color)
    {
        if(Radius == 0)
        {
            // NOTE(tcn): Seems to be the fastest way to paint a rectangle.
            const RECT Area{ Dimensions.x, Dimensions.y, Dimensions.z, Dimensions.w };
            SetBkColor(Devicecontext, Color);
            ExtTextOutW(Devicecontext, 0, 0, ETO_OPAQUE, &Area, NULL, 0, NULL);
        }
        else
        {
            const Pen_t Pen(Devicecontext, 1, Color);
            const Brush_t Brush(Devicecontext, Color);
            RoundRect(Devicecontext, Dimensions.x, Dimensions.y, Dimensions.z, Dimensions.w, Radius, Radius);
        }
    }
    void Quad_t::Solid(Texture2D Texture) { assert(false); }
    void Quad_t::Outline(uint8_t Linewidth, COLORREF Color)
    {
        const Pen_t Pen(Devicecontext, Linewidth, Color);
        const Brush_t Brush(Devicecontext, OPAQUE);

        if (Radius == 0) Rectangle(Devicecontext, Dimensions.x, Dimensions.y, Dimensions.z, Dimensions.w);
        else RoundRect(Devicecontext, Dimensions.x, Dimensions.y, Dimensions.z, Dimensions.w, Radius, Radius);
    }
    void Quad_t::Filled(uint8_t Linewidth, COLORREF Outline, COLORREF Background)
    {
        const Pen_t Pen(Devicecontext, Linewidth, Outline);
        const Brush_t Brush(Devicecontext, Background);

        if (Radius == 0) Rectangle(Devicecontext, Dimensions.x, Dimensions.y, Dimensions.z, Dimensions.w);
        else RoundRect(Devicecontext, Dimensions.x, Dimensions.y, Dimensions.z, Dimensions.w, Radius, Radius);
    }
    void Quad_t::Filled(uint8_t Linewidth, COLORREF Outline, Texture2D Background) { assert(false); }

    vec2_t Text_t::getTextsize(std::wstring_view String)
    {
        SIZE Textsize;
        SelectObject(Devicecontext, Font);

        if (GetTextExtentPoint32W(Devicecontext, String.data(), int(String.size()), &Textsize))
            return { int16_t(Textsize.cx), int16_t(Textsize.cy) };

        return {};
    }
    void Text_t::Transparent(vec2_t Position, std::wstring_view String)
    {
        SelectObject(Devicecontext, Font);
        SetTextColor(Devicecontext, Color);
        ExtTextOutW(Devicecontext, Position.x, Position.y, ETO_NUMERICSLATIN, NULL, String.data(), (UINT)String.size(), NULL);
    }
    void Text_t::Opaque(vec2_t Position, std::wstring_view String, COLORREF Background)
    {
        SelectObject(Devicecontext, Font);
        SetTextColor(Devicecontext, Color);
        SetBkColor(Devicecontext, Background);
        ExtTextOutW(Devicecontext, Position.x, Position.y, ETO_NUMERICSLATIN | ETO_OPAQUE, NULL, String.data(), (UINT)String.size(), NULL);
    }

    HFONT Text_t::getDefaultfont()
    {
        static const auto Default = Createfont("Consolas", 16);
        return Default;
    }
    HFONT Text_t::Createfont(std::string_view Name, int8_t Fontsize)
    {
        return CreateFontA(Fontsize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, Name.data());
    }
    void Text_t::setAlignment(HDC Context, bool Left, bool Right, bool Centre)
    {
        uint32_t Current = GetTextAlign(Context);
        if (Left) Current = (Current & ~TA_RIGHT) | TA_LEFT;
        if (Right) Current = (Current & ~TA_LEFT) | TA_RIGHT;
        if (Centre) Current = (Current & ~(TA_LEFT | TA_RIGHT)) | TA_CENTER;

        SetTextAlign(Context, Current);
    }
    HFONT Text_t::Createfont(std::string_view Name, int8_t Fontsize, size_t Size, void *Data)
    {
        DWORD Installedfonts{};
        (void)AddFontMemResourceEx(Data, (DWORD)Size, NULL, &Installedfonts);
        return Createfont(Name, Fontsize);
    }
}

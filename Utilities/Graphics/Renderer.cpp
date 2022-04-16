/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-07-20
    License: MIT
*/

#include <Stdinclude.hpp>
#include "Renderer.hpp"

// Windows only module.
static_assert(Build::isWindows, "Renderer is only available on Windows for now.");

namespace Graphics
{
    // RTTI helpers for the GDI state.
    struct GDIRTTI_t
    {
        HDC DC;
        HGDIOBJ Previousobject;

        GDIRTTI_t() = default;
        GDIRTTI_t(HDC Devicecontext, HGDIOBJ Currentobject) : DC(Devicecontext), Previousobject(SelectObject(DC, Currentobject)) {}
        ~GDIRTTI_t() { SelectObject(DC, Previousobject); }
    };
    struct Background_t
    {
        COLORREF Previouscolor{ 0x00FFFFFF };
        HDC DC;

        Background_t(HDC Devicecontext, std::optional<COLORREF> Color) : DC(Devicecontext)
        {
            if (Color) Previouscolor = SetBkColor(DC, *Color);
            else SetBkMode(DC, TRANSPARENT);
        }
        ~Background_t()
        {
            if (Previouscolor == 0x00FFFFFF) SetBkMode(DC, OPAQUE);
            else SetBkColor(DC, Previouscolor);
        }
    };
    struct Textcolor_t
    {
        COLORREF Previouscolor;
        HDC DC;

        explicit Textcolor_t(HDC Devicecontext, COLORREF Color) : DC(Devicecontext)
        {
            Previouscolor = SetTextColor(DC, Color);
        }
        ~Textcolor_t()
        {
            SetTextColor(DC, Previouscolor);
        }
    };

    // Helper for storage.
    static Hashmap<COLORREF, HGDIOBJ> Brushes{};
    static Hashmap<COLORREF, Hashmap<int, HGDIOBJ>> Pens{};
    static Hashmap<std::wstring, Hashmap<int, HFONT>> Fonts{};
    static const HGDIOBJ Nullbrush = GetStockObject(NULL_BRUSH);
    static const HGDIOBJ Nullpen = CreatePen(PS_NULL, 0, 0);

    static HGDIOBJ getBrush(std::optional<COLORREF> Color)
    {
        if (!Color) return Nullbrush;

        auto &Entry = Brushes[*Color];
        if (Entry == NULL) Entry = CreateSolidBrush(*Color);

        return Entry;
    }
    static HFONT getFont(const std::wstring &Name, int8_t Fontsize)
    {
        auto &Entry = Fonts[Name][Fontsize];
        if (Entry == NULL)
            Entry = CreateFontW(Fontsize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, Name.c_str());

        return Entry;
    }
    static HGDIOBJ getPen(std::optional<COLORREF> Color, int Linewidth)
    {
        if (!Color) return Nullpen;

        auto &Entry = Pens[*Color][Linewidth];
        if (Entry == NULL) Entry = CreatePen(PS_SOLID, Linewidth, *Color);

        return Entry;
    }

    static HFONT getDefaultfont()
    {
        return getFont(L"Consolas", 14);
    }

    struct Pen_t : GDIRTTI_t
    {
        Pen_t(HDC Devicecontext, std::optional<COLORREF> Color, int Linewidth) : GDIRTTI_t(Devicecontext, getPen(Color, Linewidth)) {}
    };
    struct Font_t : GDIRTTI_t
    {
        Font_t(HDC Devicecontext, HFONT Font) : GDIRTTI_t(Devicecontext, Font) {}
    };
    struct Brush_t : GDIRTTI_t
    {
        Brush_t(HDC Devicecontext, std::optional<COLORREF> Color) : GDIRTTI_t(Devicecontext, getBrush(Color)) {}
    };

    // Renderobjects may have their own state.
    struct Gradient_t : Renderobject_t
    {
        GRADIENT_RECT Indicies{ 0, 1 };
        TRIVERTEX Vertices[2]{};
        vec4i Dimensions{};
        bool Horizontal{};

        Gradient_t(HDC Context, vec2i Position, vec2i Size, bool Vertical) : Renderobject_t(Context),
                                                                             Horizontal(!Vertical)
        {
            Dimensions = { Position.x, Position.y, Position.x + Size.x, Position.y + Size.y };
        }

        void Render(std::optional<Color_t> Outline, std::optional<Color_t> Background) override
        {
            const auto O = Outline.value_or(Color_t()), B = Background.value_or(Color_t());
            Vertices[0] = { Dimensions.x, Dimensions.y, O.r, O.g, O.b, O.a };
            Vertices[1] = { Dimensions.z, Dimensions.w, B.r, B.g, B.b, B.a };

            GradientFill(Devicecontext, Vertices, 2, &Indicies, 1, Horizontal ? GRADIENT_FILL_RECT_H : GRADIENT_FILL_RECT_V);
        }
    };
    struct Ellipse_t : Renderobject_t
    {
        vec4i Dimensions;
        uint8_t Width;

        Ellipse_t(HDC Context, uint8_t Linewidth, vec2i Position, vec2i Size) : Renderobject_t(Context),
            Dimensions(Position.x, Position.y, Position.x + Size.x, Position.y + Size.y), Width(Linewidth) {}

        void Render(std::optional<Color_t> Outline, std::optional<Color_t> Background) override
        {
            const Brush_t BG(Devicecontext, Background);
            const Pen_t FG(Devicecontext, Outline, Width);
            Ellipse(Devicecontext, Dimensions.x, Dimensions.y, Dimensions.z, Dimensions.w);
        }
    };
    struct Polygon_t : Renderobject_t
    {
        std::vector<POINT> GDIPoints{};
        uint8_t Width;

        Polygon_t(HDC Context, uint8_t Linewidth, const std::vector<vec2i> &Points) : Renderobject_t(Context),
            Width(Linewidth)
        {
            GDIPoints.resize(Points.size());

            for (const auto &[Index, Point] : lz::enumerate(Points))
                GDIPoints[Index] = { Point.x, Point.y };
        }

        void Render(std::optional<Color_t> Outline, std::optional<Color_t> Background) override
        {
            const Brush_t BG(Devicecontext, Background);
            const Pen_t FG(Devicecontext, Outline, Width);
            Polygon(Devicecontext, GDIPoints.data(), (int)GDIPoints.size());
        }
    };
    struct Path_t : Renderobject_t
    {
        std::vector<POINT> GDIPoints{};
        uint8_t Width;

        Path_t(HDC Context, uint8_t Linewidth, const std::vector<vec2i> &Points) : Renderobject_t(Context),
            Width(Linewidth)
        {
            GDIPoints.resize(Points.size());

            for (const auto &[Index, Point] : lz::enumerate(Points))
                GDIPoints[Index] = { Point.x, Point.y };
        }

        void Render(std::optional<Color_t> Outline, std::optional<Color_t> Background) override
        {
            const Brush_t BG(Devicecontext, Background);
            const Pen_t FG(Devicecontext, Outline, Width);
            Polyline(Devicecontext, GDIPoints.data(), (int)GDIPoints.size());
        }
    };
    struct Mesh_t : Renderobject_t
    {
        std::vector<GRADIENT_TRIANGLE> Indicies{};
        std::vector<TRIVERTEX> Vertices{};
        uint8_t Width;

        Mesh_t(HDC Context, uint8_t Linewidth, const std::vector<vec2i> &Points, const std::vector<Color_t> &Colors) : Renderobject_t(Context)
        {
            assert(Points.size() == Colors.size()); assert(Points.size() % 3 == 0);

            Width = Linewidth;
            Vertices.resize(Points.size());
            Indicies.resize(Points.size() / 3);

            for (const auto &[Index, Tuple] : lz::enumerate(lz::zip(Points, Colors)))
            {
                const auto &[Point, Color] = Tuple;
                Indicies[Index] = { ULONG(Index), ULONG(Index + 1), ULONG(Index + 2) };
                Vertices[Index] = { Point.x, Point.y, Color.r, Color.g, Color.b, Color.a};
            }
        }

        void Render(std::optional<Color_t> Outline, std::optional<Color_t> Background) override
        {
            // Fill the shape.
            if (Background)
            {
                GradientFill(Devicecontext, Vertices.data(), (ULONG)Vertices.size(), Indicies.data(), (ULONG)Indicies.size(), GRADIENT_FILL_TRIANGLE);
            }

            // Find solid lines and draw them.
            if (Outline)
            {
                std::vector<POINT> Points;
                Points.reserve(Vertices.size());
                TRIVERTEX Lastvertex = Vertices[0];

                for (const auto &Vertex : Vertices)
                {
                    Points.emplace_back(Vertex.x, Vertex.y);

                    // Compare the color part.
                    if (*(uint64_t *)&Vertex.Red != *(uint64_t *)&Lastvertex.Red)
                    {
                        const Color_t Color(Lastvertex.Red, Lastvertex.Green, Lastvertex.Blue, Lastvertex.Alpha);
                        const Pen_t FG(Devicecontext, Width, Color);

                        // Solid lines rather than gradients.
                        Polyline(Devicecontext, Points.data(), (int)Points.size());

                        Points.clear();
                        Lastvertex = Vertex;
                        Points.emplace_back(Vertex.x, Vertex.y);
                    }
                }

                // Paint remaining.
                if (!Points.empty())
                {
                    const Color_t Color(Lastvertex.Red, Lastvertex.Green, Lastvertex.Blue, Lastvertex.Alpha);
                    const Pen_t FG(Devicecontext, Width, Color);

                    Polyline(Devicecontext, Points.data(), (int)Points.size());
                    Points.clear();
                }
            }
        }
    };
    struct Quad_t : Renderobject_t
    {
        vec4i Dimensions{};
        uint8_t Radius;
        uint8_t Width;

        Quad_t(HDC Context, vec2i Position, vec2i Size, uint8_t Rounding, uint8_t Linewidth) : Renderobject_t(Context),
            Width(Linewidth),
            Radius(Rounding)
        {
            Dimensions = { Position.x, Position.y, Position.x + Size.x, Position.y + Size.y };
            RECT Area{ Dimensions.x, Dimensions.y, Dimensions.z, Dimensions.w };
            assert((int)Dimensions.x == Position.x);
            assert((int)Dimensions.y == Position.y);
            assert((int)Dimensions.z == Position.x + Size.x);
            assert((int)Dimensions.w == Position.y + Size.y);

        }

        void Render(std::optional<Color_t> Outline, std::optional<Color_t> Background) override
        {
            // Solid color and square so we can optimize.
            if ((!Outline || Outline == Background) && Radius == 0)
            {
                const RECT Area{ Dimensions.x, Dimensions.y, Dimensions.z, Dimensions.w };
                const Background_t RTTI(Devicecontext, Background);

                ExtTextOutW(Devicecontext, 0, 0, ETO_OPAQUE, &Area, NULL, 0, NULL);
                return;
            }

            const Brush_t BG(Devicecontext, Background);
            const Pen_t FG(Devicecontext, Outline, Width);

            if (Radius == 0) Rectangle(Devicecontext, Dimensions.x, Dimensions.y, Dimensions.z, Dimensions.w);
            else RoundRect(Devicecontext, Dimensions.x, Dimensions.y, Dimensions.z, Dimensions.w, Radius, Radius);
        }
    };
    struct Text_t : Renderobject_t
    {
        bool isCentered{};
        HFONT Fonthandle{};
        vec4i Dimensions{};
        const std::wstring &Text;

        Text_t(HDC Context, vec2i Position, const std::wstring &String, HFONT Font) : Renderobject_t(Context),
            Fonthandle(Font), Dimensions{ Position, {} }, Text(String) {}
        Text_t(HDC Context, vec4i Boundingbox, const std::wstring &String, HFONT Font) : Renderobject_t(Context),
            isCentered(true),
            Fonthandle(Font), Dimensions(Boundingbox), Text(String) {}

        void Render(std::optional<Color_t> Outline, std::optional<Color_t> Background) override
        {
            const RECT Area{ Dimensions.x, Dimensions.y, Dimensions.z, Dimensions.w };
            const Textcolor_t RTTI1(Devicecontext, Outline.value_or(Color_t()));
            const RECT *pArea = (Dimensions.z && Dimensions.w) ? &Area : NULL;
            const Font_t RTTI2(Devicecontext, Fonthandle);
            [[maybe_unused]] UINT Alignment{};

            if (isCentered)
            {
                Alignment = SetTextAlign(Devicecontext, TA_CENTER);
            }

            if (Background)
            {
                const Background_t RTTI3(Devicecontext, Background);
                ExtTextOutW(Devicecontext, Dimensions.x, Dimensions.y, ETO_NUMERICSLATIN | ETO_OPAQUE, pArea, Text.c_str(), (UINT)Text.size(), NULL);
            }
            else
            {
                ExtTextOutW(Devicecontext, Dimensions.x, Dimensions.y, ETO_NUMERICSLATIN, pArea, Text.c_str(), (UINT)Text.size(), NULL);
            }

            if (isCentered)
            {
                SetTextAlign(Devicecontext, Alignment);
            }
        }
    };
    struct Arc_t : Renderobject_t
    {
        vec2i Focalpoint0, Focalpoint1;
        vec4i Boundingbox;
        uint8_t Width;

        Arc_t(HDC Context, uint8_t Linewidth, vec2i Position, vec2i Angles, uint8_t Rounding) : Renderobject_t(Context),
            Width(Linewidth)
        {
            Boundingbox = { Position.x - Rounding, Position.y - Rounding, Position.x + Rounding, Position.y + Rounding };

            Focalpoint0.x = Position.x + Rounding * std::cosf(float((Angles.x + Angles.y) * std::numbers::pi / 180.0f));
            Focalpoint0.y = Position.y + Rounding * std::sinf(float((Angles.x + Angles.y) * std::numbers::pi / 180.0f));
            Focalpoint1.x = Position.x + Rounding * std::cosf(float(Angles.x * std::numbers::pi / 180.0f));
            Focalpoint1.y = Position.y + Rounding * std::sinf(float(Angles.y * std::numbers::pi / 180.0f));
        }

        void Render(std::optional<Color_t> Outline, std::optional<Color_t> Background) override
        {
            const Brush_t BG(Devicecontext, Background);
            const Pen_t FG(Devicecontext, Outline, Width);
            SetArcDirection(Devicecontext, AD_COUNTERCLOCKWISE);

            if (!Background)
            {
                Arc(Devicecontext, Boundingbox.x, Boundingbox.y, Boundingbox.z, Boundingbox.w,
                    Focalpoint0.x, Focalpoint0.y, Focalpoint1.x, Focalpoint1.y);
            }
            else
            {
                Pie(Devicecontext, Boundingbox.x, Boundingbox.y, Boundingbox.z, Boundingbox.w,
                    Focalpoint0.x, Focalpoint0.y, Focalpoint1.x, Focalpoint1.y);
            }
        }
    };

    // Extension-point for when PMR works properly on MSVC.
    template <typename T, typename ... Args>
    static std::unique_ptr<Renderobject_t> Internalalloc(Args&& ...args)
    {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }

    // Create renderobjects based on the users needs, only valid until the next call.
    std::unique_ptr<Renderobject_t> Renderer_t::Line(vec2i Start, vec2i Stop, uint8_t Linewidth) const noexcept
    {
        std::vector<vec2i> TMP{ Start, Stop };
        return Internalalloc<Path_t>(Devicecontext, Linewidth, TMP);
    }
    std::unique_ptr<Renderobject_t> Renderer_t::Ellipse(vec2i Position, vec2i Size, uint8_t Linewidth) const noexcept
    {
        return Internalalloc<Ellipse_t>(Devicecontext, Linewidth, Position, Size);
    }
    std::unique_ptr<Renderobject_t> Renderer_t::Gradientrect(vec2i Position, vec2i Size, bool Vertical) const noexcept
    {
        return Internalalloc<Gradient_t>(Devicecontext, Position, Size, Vertical);
    }
    std::unique_ptr<Renderobject_t> Renderer_t::Path(const std::vector<vec2i> &Points, uint8_t Linewidth) const noexcept
    {
        return Internalalloc<Path_t>(Devicecontext, Linewidth, Points);
    }
    std::unique_ptr<Renderobject_t> Renderer_t::Polygon(const std::vector<vec2i> &Points, uint8_t Linewidth) const noexcept
    {
        return Internalalloc<Polygon_t>(Devicecontext, Linewidth, Points);
    }
    std::unique_ptr<Renderobject_t> Renderer_t::Arc(vec2i Position, vec2i Angles, uint8_t Rounding, uint8_t Linewidth) const noexcept
    {
        return Internalalloc<Arc_t>(Devicecontext, Linewidth, Position, Angles, Rounding);
    }
    std::unique_ptr<Renderobject_t> Renderer_t::Rectangle(vec2i Position, vec2i Size, uint8_t Rounding, uint8_t Linewidth) const noexcept
    {
        return Internalalloc<Quad_t>(Devicecontext, Position, Size, Rounding, Linewidth);
    }
    std::unique_ptr<Renderobject_t> Renderer_t::Mesh(const std::vector<vec2i> &Points, const std::vector<Color_t> &Colors, uint8_t Linewidth) const noexcept
    {
        return Internalalloc<Mesh_t>(Devicecontext, Linewidth, Points, Colors);
    }

    std::unique_ptr<Renderobject_t> Renderer_t::Text(vec2i Position, const std::wstring &String, HFONT Font) const noexcept
    {
        return Internalalloc<Text_t>(Devicecontext, Position, String, Font == NULL ? getDefaultfont() : Font);
    }
    std::unique_ptr<Renderobject_t> Renderer_t::Textcentered(vec4i Boundingbox, const std::wstring &String, HFONT Font) const noexcept
    {
        return Internalalloc<Text_t>(Devicecontext, Boundingbox, String, Font == NULL ? getDefaultfont() : Font);
    }
}

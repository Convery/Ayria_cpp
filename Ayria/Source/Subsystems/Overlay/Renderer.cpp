/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-06-14
    License: MIT
*/

#define STB_IMAGE_IMPLEMENTATION
#include <Stdinclude.hpp>
#include <Global.hpp>

namespace Graphics
{
    // Rather than allocating for each object, just store the largest and overwrite.
    enum Objectypes { NONE, PATH, ELLIPSE, ARC, MESH, GRADIENT, POLYGON, QUAD, TEXT };
    static uint8_t Renderobject[60 /*sizeof(Gradient_t)*/];
    static Objectypes Currentobject{};

    // Renderobjects may have their own state.
    struct Gradient_t : Renderobject_t
    {
        GRADIENT_RECT Indicies{ 0, 1 };
        TRIVERTEX Vertices[2]{};
        vec4f Dimensions{};
        bool Horizontal{};

        Gradient_t(HDC Context, vec2f Position, vec2f Size, bool Vertical) : Renderobject_t(Context)
        {
            Dimensions = { Position.x, Position.y, Position.x + Size.x, Position.y + Size.y };
            Currentobject = GRADIENT;
            Horizontal = !Vertical;
        }
        virtual void Render(std::optional<Color_t> Outline, std::optional<Color_t> Background)
        {
            const auto O = Outline.value_or(Color_t()), B = Background.value_or(Color_t());
            Vertices[0] = { Dimensions.x, Dimensions.y, O.r, O.g, O.b, O.a };
            Vertices[1] = { Dimensions.z, Dimensions.w, B.r, B.g, B.b, B.a };

            GradientFill(Devicecontext, Vertices, 2, &Indicies, 1, Horizontal ? GRADIENT_FILL_RECT_H : GRADIENT_FILL_RECT_V);
        }
    };
    struct Ellipse_t : Renderobject_t
    {
        vec4f Dimensions;
        uint8_t Width;

        Ellipse_t(HDC Context, uint8_t Linewidth, vec2f Position, vec2f Size) : Renderobject_t(Context)
        {
            Dimensions = { Position.x, Position.y, Position.x + Size.x, Position.y + Size.y };
            Currentobject = ELLIPSE;
            Width = Linewidth;
        }
        virtual void Render(std::optional<Color_t> Outline, std::optional<Color_t> Background)
        {
            const Brush_t BG(Devicecontext, Background.value_or(Color_t(uint32_t(OPAQUE))));
            const Pen_t FG(Devicecontext, Width, Outline.value_or(Color_t(uint32_t(OPAQUE))));
            Ellipse(Devicecontext, Dimensions.x, Dimensions.y, Dimensions.z, Dimensions.w);
        }
    };
    struct Polygon_t : Renderobject_t
    {
        std::vector<POINT> GDIPoints;
        uint8_t Width;

        Polygon_t(HDC Context, uint8_t Linewidth, const std::vector<vec2f> &Points) : Renderobject_t(Context)
        {
            Width = Linewidth;
            Currentobject = POLYGON;
            GDIPoints.resize(Points.size());

            for (const auto &[Index, Point] : lz::enumerate(Points))
                GDIPoints[Index] = { Point.x, Point.y };
        }
        virtual void Render(std::optional<Color_t> Outline, std::optional<Color_t> Background)
        {
            const Brush_t BG(Devicecontext, Background.value_or(Color_t(uint32_t(OPAQUE))));
            const Pen_t FG(Devicecontext, Width, Outline.value_or(Color_t(uint32_t(OPAQUE))));
            Polygon(Devicecontext, GDIPoints.data(), (int)GDIPoints.size());
        }
    };
    struct Path_t : Renderobject_t
    {
        std::vector<POINT> GDIPoints;
        uint8_t Width;

        Path_t(HDC Context, uint8_t Linewidth, const std::vector<vec2f> &Points) : Renderobject_t(Context)
        {
            Width = Linewidth;
            Currentobject = PATH;
            GDIPoints.resize(Points.size());

            for (const auto &[Index, Point] : lz::enumerate(Points))
                GDIPoints[Index] = { Point.x, Point.y };
        }
        virtual void Render(std::optional<Color_t> Outline, std::optional<Color_t> Background)
        {
            const Brush_t BG(Devicecontext, Background.value_or(Color_t(uint32_t(OPAQUE))));
            const Pen_t FG(Devicecontext, Width, Outline.value_or(Color_t(uint32_t(OPAQUE))));
            Polyline(Devicecontext, GDIPoints.data(), (int)GDIPoints.size());
        }
    };
    struct Mesh_t : Renderobject_t
    {
        std::vector<GRADIENT_TRIANGLE> Indicies;
        std::vector<TRIVERTEX> Vertices;
        uint8_t Width;

        Mesh_t(HDC Context, uint8_t Linewidth, const std::vector<vec2f> &Points, const std::vector<Color_t> &Colors) : Renderobject_t(Context)
        {
            assert(Points.size() == Colors.size()); assert(Points.size() % 3 == 0);

            Width = Linewidth;
            Currentobject = MESH;
            Vertices.resize(Points.size());
            Indicies.resize(Points.size() / 3);

            for (const auto &[Index, Tuple] : lz::enumerate(lz::zip(Points, Colors)))
            {
                const auto &[Point, Color] = Tuple;
                Indicies[Index] = { ULONG(Index), ULONG(Index + 1), ULONG(Index + 2) };
                Vertices[Index] = { Point.x, Point.y, Color.r, Color.g, Color.b, Color.a};
            }
        }
        virtual void Render(std::optional<Color_t> Outline, std::optional<Color_t> Background)
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
                    if (0 != std::memcmp(&Vertex.Red, &Lastvertex.Red, sizeof(COLOR16) * 4))
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
        vec4f Dimensions{};
        uint8_t Radius;
        uint8_t Width;

        Quad_t(HDC Context, vec2f Position, vec2f Size, uint8_t Rounding, uint8_t Linewidth) : Renderobject_t(Context)
        {
            Dimensions = { Position.x, Position.y, Position.x + Size.x, Position.y + Size.y };
            Currentobject = QUAD;
            Radius = Rounding;
            Width = Linewidth;
        }
        virtual void Render(std::optional<Color_t> Outline, std::optional<Color_t> Background)
        {
            // Solid color and square so we can optimize.
            if ((!Outline || Outline == Background) && Radius == 0)
            {
                const RECT Area{ Dimensions.x, Dimensions.y, Dimensions.z, Dimensions.w };
                const Graphics::Backgroundcolor_t RTTI(Devicecontext, Background.value_or(Color_t()));

                ExtTextOutW(Devicecontext, 0, 0, ETO_OPAQUE, &Area, NULL, 0, NULL);
                return;
            }

            const Pen_t FG(Devicecontext, Width, Outline.value_or(Color_t(uint32_t(OPAQUE))));
            const Brush_t BG(Devicecontext, Background.value_or(Color_t(uint32_t(OPAQUE))));

            if (Radius == 0) Rectangle(Devicecontext, Dimensions.x, Dimensions.y, Dimensions.z, Dimensions.w);
            else RoundRect(Devicecontext, Dimensions.x, Dimensions.y, Dimensions.z, Dimensions.w, Radius, Radius);
        }
    };
    struct Text_t : Renderobject_t
    {
        bool isCentered{};
        HFONT Fonthandle{};
        vec4f Dimensions{};
        std::wstring Text{};

        Text_t(HDC Context, vec2f Position, const std::wstring &String, HFONT Font) : Renderobject_t(Context)
        {
            Currentobject = TEXT;

            Dimensions = { Position, {} };
            isCentered = false;
            Fonthandle = Font;
            Text = String;
        }
        Text_t(HDC Context, vec4f Boundingbox, const std::wstring &String, HFONT Font) : Renderobject_t(Context)
        {
            Currentobject = TEXT;

            Dimensions = Boundingbox;
            Fonthandle = Font;
            isCentered = true;
            Text = String;
        }

        virtual void Render(std::optional<Color_t> Outline, std::optional<Color_t> Background)
        {
            const RECT Area{ Dimensions.x, Dimensions.y, Dimensions.z, Dimensions.w };
            const RECT *pArea = (Dimensions.z && Dimensions.w) ? &Area : NULL;
            const Textcolor_t RTTI1(Devicecontext, Outline.value_or(Color_t()));
            const auto Alignment = GetTextAlign(Devicecontext);
            const Font_t RTTI2(Devicecontext, Fonthandle);

            if (isCentered)
            {
                const uint32_t Newalign = (Alignment & ~TA_RIGHT) | TA_LEFT;
                SetTextAlign(Devicecontext, Newalign);
            }

            if (Background)
            {
                const Backgroundcolor_t RTTI3(Devicecontext, Background.value_or(Color_t()));
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
        vec2f Focalpoint0, Focalpoint1;
        vec4f Boundingbox;
        uint8_t Width;

         Arc_t(HDC Context, uint8_t Linewidth, vec2f Position, vec2f Angles, uint8_t Rounding) : Renderobject_t(Context)
        {
            Width = Linewidth;
            Currentobject = ARC;
            Boundingbox = { Position.x - Rounding, Position.y - Rounding, Position.x + Rounding, Position.y + Rounding };

            Focalpoint0.x = Position.x + Rounding * std::cosf(float((Angles.x + Angles.y) * std::numbers::pi / 180.0f));
            Focalpoint0.y = Position.y + Rounding * std::sinf(float((Angles.x + Angles.y) * std::numbers::pi / 180.0f));
            Focalpoint1.x = Position.x + Rounding * std::cosf(float(Angles.x * std::numbers::pi / 180.0f));
            Focalpoint1.y = Position.y + Rounding * std::sinf(float(Angles.y * std::numbers::pi / 180.0f));
        }
        virtual void Render(std::optional<Color_t> Outline, std::optional<Color_t> Background)
        {
            const Pen_t FG(Devicecontext, Width, Outline.value_or(Color_t(uint32_t(OPAQUE))));
            const Brush_t BG(Devicecontext, Background.value_or(Color_t(uint32_t(OPAQUE))));
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

    // Renderobject management.
    static void Internalclear()
    {
        // Most are no-ops but better be consistent.
        switch (Currentobject)
        {
            case GRADIENT: ((Gradient_t *)Renderobject)->~Gradient_t(); break;
            case ELLIPSE: ((Ellipse_t *)Renderobject)->~Ellipse_t(); break;
            case POLYGON: ((Polygon_t *)Renderobject)->~Polygon_t(); break;
            case PATH: ((Path_t *)Renderobject)->~Path_t(); break;
            case MESH: ((Mesh_t *)Renderobject)->~Mesh_t(); break;
            case TEXT: ((Text_t *)Renderobject)->~Text_t(); break;
            case QUAD: ((Quad_t *)Renderobject)->~Quad_t(); break;
            case ARC: ((Arc_t *)Renderobject)->~Arc_t(); break;

            default: break;
        }

        Currentobject = NONE;
    }
    Renderer_t::~Renderer_t() { Internalclear(); if (Clipping) DeleteObject(Clipping); }
    template <typename T, typename ... Args> static Renderobject_t *Internalalloc(Args& ...args)
    {
        Internalclear();
        return new (Renderobject) T(std::forward<Args>(args)...);
    }

    // Create renderobjects based on the users needs, only valid until the next call.
    Renderobject_t *Renderer_t::Line(vec2f Start, vec2f Stop, uint8_t Linewidth) const noexcept
    {
        std::vector<vec2f> TMP{ Start, Stop };
        return Internalalloc<Path_t>(Devicecontext, Linewidth, TMP);
    }
    Renderobject_t *Renderer_t::Ellipse(vec2f Position, vec2f Size, uint8_t Linewidth) const noexcept
    {
        return Internalalloc<Ellipse_t>(Devicecontext, Linewidth, Position, Size);
    }
    Renderobject_t *Renderer_t::Gradientrect(vec2f Position, vec2f Size, bool Vertical) const noexcept
    {
        return Internalalloc<Gradient_t>(Devicecontext, Position, Size, Vertical);
    }
    Renderobject_t *Renderer_t::Path(const std::vector<vec2f> &Points, uint8_t Linewidth) const noexcept
    {
        return Internalalloc<Path_t>(Devicecontext, Linewidth, Points);
    }
    Renderobject_t *Renderer_t::Polygon(const std::vector<vec2f> &Points, uint8_t Linewidth) const noexcept
    {
        return Internalalloc<Polygon_t>(Devicecontext, Linewidth, Points);
    }
    Renderobject_t *Renderer_t::Arc(vec2f Position, vec2f Angles, uint8_t Rounding, uint8_t Linewidth) const noexcept
    {
        return Internalalloc<Arc_t>(Devicecontext, Linewidth, Position, Angles, Rounding);
    }
    Renderobject_t *Renderer_t::Rectangle(vec2f Position, vec2f Size, uint8_t Rounding, uint8_t Linewidth) const noexcept
    {
        return Internalalloc<Quad_t>(Devicecontext, Position, Size, Rounding, Linewidth);
    }
    Renderobject_t *Renderer_t::Mesh(const std::vector<vec2f> &Points, const std::vector<Color_t> &Colors, uint8_t Linewidth) const noexcept
    {
        return Internalalloc<Mesh_t>(Devicecontext, Linewidth, Points, Colors);
    }

    Renderobject_t *Renderer_t::Text(vec2f Position, const std::wstring &String, HFONT Font) const noexcept
    {
        return Internalalloc<Text_t>(Devicecontext, Position, String, Font);
    }
    Renderobject_t *Renderer_t::Textcentered(vec4f Boundingbox, const std::wstring &String, HFONT Font) const noexcept
    {
        return Internalalloc<Text_t>(Devicecontext, Boundingbox, String, Font);
    }
}

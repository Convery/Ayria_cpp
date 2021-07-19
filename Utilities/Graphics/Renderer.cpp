/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-07-20
    License: MIT
*/

#define STB_IMAGE_IMPLEMENTATION
#include <Stdinclude.hpp>
#include "../Datatypes.hpp"
#include "Renderer.hpp"
#include "Overlay.hpp"

// Windows only module.
static_assert(Build::isWindows, "Renderer is only available on Windows for now.");

namespace Graphics
{
    // While Ayria currently only uses a single instance, better be safe.
    thread_local std::shared_ptr<Renderobject_t> Currentobject{};
    thread_local std::pmr::monotonic_buffer_resource Pool{};

    // Always clear the pool when destroying.
    Renderobject_t::~Renderobject_t()
    {
        Pool.release();
    }

    // Renderobjects may have their own state.
    struct Gradient_t : Renderobject_t
    {
        GRADIENT_RECT Indicies{ 0, 1 };
        TRIVERTEX Vertices[2]{};
        vec4f Dimensions{};
        bool Horizontal{};

        Gradient_t(HDC Context, vec2i Position, vec2i Size, bool Vertical) : Renderobject_t(Context)
        {
            Dimensions = { Position.x, Position.y, Position.x + Size.x, Position.y + Size.y };
            Horizontal = !Vertical;
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
        vec4f Dimensions;
        uint8_t Width;

        Ellipse_t(HDC Context, uint8_t Linewidth, vec2i Position, vec2i Size) : Renderobject_t(Context)
        {
            Dimensions = { Position.x, Position.y, Position.x + Size.x, Position.y + Size.y };
            Width = Linewidth;
        }

        void Render(std::optional<Color_t> Outline, std::optional<Color_t> Background) override
        {
            const Brush_t BG(Devicecontext, Background.value_or(Color_t(uint32_t(OPAQUE))));
            const Pen_t FG(Devicecontext, Width, Outline.value_or(Color_t(uint32_t(OPAQUE))));
            Ellipse(Devicecontext, Dimensions.x, Dimensions.y, Dimensions.z, Dimensions.w);
        }
    };
    struct Polygon_t : Renderobject_t
    {
        std::pmr::vector<POINT> GDIPoints{ &Pool };
        uint8_t Width;

        Polygon_t(HDC Context, uint8_t Linewidth, const std::vector<vec2i> &Points) : Renderobject_t(Context)
        {
            Width = Linewidth;
            GDIPoints.resize(Points.size());

            for (const auto &[Index, Point] : lz::enumerate(Points))
                GDIPoints[Index] = { Point.x, Point.y };
        }

        void Render(std::optional<Color_t> Outline, std::optional<Color_t> Background) override
        {
            const Pen_t FG(Devicecontext, Width, Outline.value_or(Background.value_or(Color_t(uint32_t(OPAQUE)))));
            const Brush_t BG(Devicecontext, Background.value_or(Color_t(uint32_t(OPAQUE))));
            Polygon(Devicecontext, GDIPoints.data(), (int)GDIPoints.size());
        }
    };
    struct Path_t : Renderobject_t
    {
        std::pmr::vector<POINT> GDIPoints{ &Pool };
        uint8_t Width;

        Path_t(HDC Context, uint8_t Linewidth, const std::vector<vec2i> &Points) : Renderobject_t(Context)
        {
            Width = Linewidth;
            GDIPoints.resize(Points.size());

            for (const auto &[Index, Point] : lz::enumerate(Points))
                GDIPoints[Index] = { Point.x, Point.y };
        }

        void Render(std::optional<Color_t> Outline, std::optional<Color_t> Background) override
        {
            const Brush_t BG(Devicecontext, Background.value_or(Color_t(uint32_t(OPAQUE))));
            const Pen_t FG(Devicecontext, Width, Outline.value_or(Color_t(uint32_t(OPAQUE))));
            Polyline(Devicecontext, GDIPoints.data(), (int)GDIPoints.size());
        }
    };
    struct Mesh_t : Renderobject_t
    {
        std::pmr::vector<GRADIENT_TRIANGLE> Indicies{ &Pool };
        std::pmr::vector<TRIVERTEX> Vertices{ &Pool };
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

        Quad_t(HDC Context, vec2i Position, vec2i Size, uint8_t Rounding, uint8_t Linewidth) : Renderobject_t(Context)
        {
            Dimensions = { Position.x, Position.y, Position.x + Size.x, Position.y + Size.y };
            Radius = Rounding;
            Width = Linewidth;
        }

        void Render(std::optional<Color_t> Outline, std::optional<Color_t> Background) override
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
        std::pmr::wstring Text{ &Pool };

        Text_t(HDC Context, vec2i Position, const std::wstring &String, HFONT Font) : Renderobject_t(Context)
        {
            Dimensions = { Position, {} };
            isCentered = false;
            Fonthandle = Font;
            Text = String;
        }
        Text_t(HDC Context, vec4f Boundingbox, const std::wstring &String, HFONT Font) : Renderobject_t(Context)
        {
            Dimensions = Boundingbox;
            Fonthandle = Font;
            isCentered = true;
            Text = String;
        }

        void Render(std::optional<Color_t> Outline, std::optional<Color_t> Background) override
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
        vec2i Focalpoint0, Focalpoint1;
        vec4f Boundingbox;
        uint8_t Width;

        Arc_t(HDC Context, uint8_t Linewidth, vec2i Position, vec2i Angles, uint8_t Rounding) : Renderobject_t(Context)
        {
            Width = Linewidth;
            Boundingbox = { Position.x - Rounding, Position.y - Rounding, Position.x + Rounding, Position.y + Rounding };

            Focalpoint0.x = Position.x + Rounding * std::cosf(float((Angles.x + Angles.y) * std::numbers::pi / 180.0f));
            Focalpoint0.y = Position.y + Rounding * std::sinf(float((Angles.x + Angles.y) * std::numbers::pi / 180.0f));
            Focalpoint1.x = Position.x + Rounding * std::cosf(float(Angles.x * std::numbers::pi / 180.0f));
            Focalpoint1.y = Position.y + Rounding * std::sinf(float(Angles.y * std::numbers::pi / 180.0f));
        }

        void Render(std::optional<Color_t> Outline, std::optional<Color_t> Background) override
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

    // Helper to reassign the pointer.
    template <typename T, typename ... Args>
    static std::shared_ptr<Renderobject_t> Internalalloc(Args& ...args)
    {
        Currentobject = std::make_shared<T>(std::forward<Args>(args)...);
        return Currentobject;
    }

    // Create renderobjects based on the users needs, only valid until the next call.
    std::shared_ptr<Renderobject_t> Renderer_t::Line(vec2i Start, vec2i Stop, uint8_t Linewidth) const noexcept
    {
        std::vector<vec2i> TMP{ Start, Stop };
        return Internalalloc<Path_t>(Devicecontext, Linewidth, TMP);
    }
    std::shared_ptr<Renderobject_t> Renderer_t::Ellipse(vec2i Position, vec2i Size, uint8_t Linewidth) const noexcept
    {
        return Internalalloc<Ellipse_t>(Devicecontext, Linewidth, Position, Size);
    }
    std::shared_ptr<Renderobject_t> Renderer_t::Gradientrect(vec2i Position, vec2i Size, bool Vertical) const noexcept
    {
        return Internalalloc<Gradient_t>(Devicecontext, Position, Size, Vertical);
    }
    std::shared_ptr<Renderobject_t> Renderer_t::Path(const std::vector<vec2i> &Points, uint8_t Linewidth) const noexcept
    {
        return Internalalloc<Path_t>(Devicecontext, Linewidth, Points);
    }
    std::shared_ptr<Renderobject_t> Renderer_t::Polygon(const std::vector<vec2i> &Points, uint8_t Linewidth) const noexcept
    {
        return Internalalloc<Polygon_t>(Devicecontext, Linewidth, Points);
    }
    std::shared_ptr<Renderobject_t> Renderer_t::Arc(vec2i Position, vec2i Angles, uint8_t Rounding, uint8_t Linewidth) const noexcept
    {
        return Internalalloc<Arc_t>(Devicecontext, Linewidth, Position, Angles, Rounding);
    }
    std::shared_ptr<Renderobject_t> Renderer_t::Rectangle(vec2i Position, vec2i Size, uint8_t Rounding, uint8_t Linewidth) const noexcept
    {
        return Internalalloc<Quad_t>(Devicecontext, Position, Size, Rounding, Linewidth);
    }
    std::shared_ptr<Renderobject_t> Renderer_t::Mesh(const std::vector<vec2i> &Points, const std::vector<Color_t> &Colors, uint8_t Linewidth) const noexcept
    {
        return Internalalloc<Mesh_t>(Devicecontext, Linewidth, Points, Colors);
    }

    std::shared_ptr<Renderobject_t> Renderer_t::Text(vec2i Position, const std::wstring &String, HFONT Font) const noexcept
    {
        return Internalalloc<Text_t>(Devicecontext, Position, String, Font);
    }
    std::shared_ptr<Renderobject_t> Renderer_t::Textcentered(vec4f Boundingbox, const std::wstring &String, HFONT Font) const noexcept
    {
        return Internalalloc<Text_t>(Devicecontext, Boundingbox, String, Font);
    }
}

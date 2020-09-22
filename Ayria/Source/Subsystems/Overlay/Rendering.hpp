/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-09-21
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>
#include <Global.hpp>

#pragma pack(push, 1)
struct Texture2D
{
    HBITMAP Internalbitmap;
    vec2_t Internalsize;

    // Windows wants to use BGR for bitmaps, probably some Win16 reasoning.
    static void RGBA_SwapRB(size_t Size, void *Data);
    static void RGB_SwapRB(size_t Size, void *Data);

    explicit Texture2D(std::string_view Filepath);
    explicit Texture2D(vec2_t Dimensions, size_t Size, const void *Data) : Internalsize(Dimensions)
    {
        assert(Dimensions.x); assert(Dimensions.y); assert(Size); assert(Data);
        const short Pixelsize{ Size / (Dimensions.x * Dimensions.y) };
        BITMAPINFO BMI{ sizeof(BITMAPINFOHEADER) };
        uint8_t *DIBPixels{};

        // Windows likes to draw bitmaps upside down, so negative height.
        BMI.bmiHeader.biSizeImage = Dimensions.y * Dimensions.x * Pixelsize;
        BMI.bmiHeader.biHeight = (-1) * Dimensions.y;
        BMI.bmiHeader.biBitCount = Pixelsize * 8;
        BMI.bmiHeader.biWidth = Dimensions.x;
        BMI.bmiHeader.biCompression = BI_RGB;
        BMI.bmiHeader.biPlanes = 1;

        // Allocate the bitmap in system-memory.
        Internalbitmap = CreateDIBSection(NULL, &BMI, DIB_RGB_COLORS, (void **)&DIBPixels, NULL, 0);
        std::memcpy(DIBPixels, Data, Size);

        // TODO(tcn): Perform RGB_SwapRB as needed.

        // Re-associate the memory with our bitmap.
        SetDIBits(NULL, Internalbitmap, 0, static_cast<UINT>(Dimensions.y), DIBPixels, &BMI, DIB_RGB_COLORS);
    }
};

namespace gInternal
{
    struct Arc_t
    {
        vec2_t Focalpoint0, Focalpoint1;
        vec4_t Boundingbox;
        HDC Devicecontext;

        void Solid(COLORREF Color);
        void Solid(Texture2D Texture);
        void Outline(uint8_t Linewidth, COLORREF Color);
        void Filled(uint8_t Linewidth, COLORREF Outline, COLORREF Background);
        void Filled(uint8_t Linewidth, COLORREF Outline, Texture2D Background);

        explicit Arc_t(HDC Context, vec2_t Position, vec2_t Angles, uint8_t Rounding) : Devicecontext(Context),
            Boundingbox{ Position.x - Rounding, Position.y - Rounding, Position.x + Rounding, Position.y + Rounding }
        {
            Focalpoint0.x = Position.x + Rounding * std::cosf(float((Angles.x + Angles.y) * std::numbers::pi / 180.0f));
            Focalpoint0.y = Position.y + Rounding * std::sinf(float((Angles.x + Angles.y) * std::numbers::pi / 180.0f));
            Focalpoint1.x = Position.x + Rounding * std::cosf(float(Angles.x * std::numbers::pi / 180.0f));
            Focalpoint1.y = Position.y + Rounding * std::sinf(float(Angles.y * std::numbers::pi / 180.0f));
        }
    };
    struct Ellipse_t
    {
        vec4_t Dimensions;
        HDC Devicecontext;

        void Solid(COLORREF Color);
        void Solid(Texture2D Texture);
        void Outline(uint8_t Linewidth, COLORREF Color);
        void Filled(uint8_t Linewidth, COLORREF Outline, COLORREF Background);
        void Filled(uint8_t Linewidth, COLORREF Outline, Texture2D Background);

        explicit Ellipse_t(HDC Context, vec2_t Position, vec2_t Size) : Devicecontext(Context),
            Dimensions{ Position.x, Position.y, Position.x + Size.x, Position.y + Size.y } {}
    };
    struct Mesh_t
    {
        std::unique_ptr<GRADIENT_TRIANGLE[]> Indicies;
        std::unique_ptr<TRIVERTEX[]> Vertices;
        uint32_t Vertexcount, Indexcount;
        HDC Devicecontext;

        void Solid();
        void Filled(uint8_t Linewidth);
        void Outline(uint8_t Linewidth);

        explicit Mesh_t(HDC Context, std::vector<POINT> Points, std::vector<COLORREF> Colors) : Devicecontext(Context)
        {
            assert(Points.size() == Colors.size()); assert(Points.size() == 4); // Malformed quad.
            const auto toVertex = [](POINT P, COLORREF C) -> TRIVERTEX
            {
                return { P.x, P.y,
                    static_cast<COLOR16>((C & 0xFF) << 8),
                    static_cast<COLOR16>((C & 0xFF00) << 8),
                    static_cast<COLOR16>((C & 0xFF0000) << 8),
                    static_cast<COLOR16>((C & 0xFF000000) << 8) };
            };

            Vertexcount = (uint32_t)Points.size();
            Indexcount = (uint32_t)Points.size() / 3;
            Vertices = std::make_unique<TRIVERTEX[]>(Vertexcount);
            Indicies = std::make_unique<GRADIENT_TRIANGLE[]>(Indexcount);
            for (ULONG i = 0; i < Indexcount; ++i) Indicies[i] = { i, i + 1, i + 2 };
            for (size_t i = 0; i < Vertexcount; ++i) Vertices[i] = toVertex(Points[i], Colors[i]);
        }
    };
    struct Path_t
    {
        std::unique_ptr<POINT[]> Points;
        uint32_t Pointcount;
        HDC Devicecontext;

        void Solid(COLORREF Color);
        void Solid(Texture2D Texture);
        void Outline(uint8_t Linewidth, COLORREF Color);
        void Filled(uint8_t Linewidth, COLORREF Outline, COLORREF Background);
        void Filled(uint8_t Linewidth, COLORREF Outline, Texture2D Background);

        explicit Path_t(HDC Context, std::vector<vec2_t> vPoints) : Devicecontext(Context)
        {
            Points = std::make_unique<POINT[]>(vPoints.size());
            Pointcount = (uint32_t)vPoints.size();

            for (size_t i = 0; i < vPoints.size(); ++i)
            {
                Points[i].x = vPoints[i].x;
                Points[i].y = vPoints[i].y;
            }
        }
        explicit Path_t(HDC Context, vec2_t Startvec, vec2_t Endvec) : Devicecontext(Context)
        {
            Points = std::make_unique<POINT[]>(2);
            Pointcount = 2;

            Points[0] = { Startvec.x, Startvec.y };
            Points[1] = { Endvec.x, Endvec.y };
        }
    };
    struct Quad_t
    {
        vec4_t Dimensions;
        HDC Devicecontext;
        uint8_t Radius;

        void Solid(COLORREF Color);
        void Solid(Texture2D Texture);
        void Outline(uint8_t Linewidth, COLORREF Color);
        void Filled(uint8_t Linewidth, COLORREF Outline, COLORREF Background);
        void Filled(uint8_t Linewidth, COLORREF Outline, Texture2D Background);

        explicit Quad_t(HDC Context, vec2_t Position, vec2_t Size, uint8_t Rounding) : Devicecontext(Context),
            Radius(Rounding), Dimensions{ Position.x, Position.y, Position.x + Size.x - 1, Position.y + Size.y - 1 } {}
    };
    struct Text_t
    {
        HDC Devicecontext;
        COLORREF Color;
        HFONT Font;

        vec2_t getTextsize(std::wstring_view String);
        void Transparent(vec2_t Position, std::wstring_view String);
        void Opaque(vec2_t Position, std::wstring_view String, COLORREF Background);

        static HFONT getDefaultfont();
        static HFONT Createfont(std::string_view Name, int8_t Fontsize);
        static void setAlignment(HDC Context, bool Left, bool Right, bool Centre);
        static HFONT Createfont(std::string_view Name, int8_t Fontsize, size_t Size, void *Data);

        explicit Text_t(HDC Context, COLORREF Textcolor) : Devicecontext(Context), Color(Textcolor), Font(getDefaultfont()) {}
        explicit Text_t(HDC Context, COLORREF Textcolor, HFONT Fonthandle) : Devicecontext(Context), Color(Textcolor), Font(Fonthandle) {}
    };
}

struct Graphics
{
    HDC Devicecontext;

    explicit Graphics(HDC Context) : Devicecontext(Context) {}
    explicit Graphics(HDC Context, vec4_t Boundingbox) : Devicecontext(Context)
    {
        SelectClipRgn(Devicecontext, CreateRectRgn(Boundingbox.x, Boundingbox.y, Boundingbox.z, Boundingbox.w));
    }

    // Helper to clear the context.
    void Clear(vec2_t Size)
    {
        SetBkColor(Devicecontext, Clearcolor);
        const RECT Clientarea{ 0, 0, Size.x, Size.y };
        ExtTextOutW(Devicecontext, 0, 0, ETO_OPAQUE, &Clientarea, NULL, 0, NULL);
    }

    gInternal::Text_t Text(COLORREF Textcolor, HFONT Fonthandle)
    {
        return gInternal::Text_t(Devicecontext, Textcolor, Fonthandle);
    }
    gInternal::Ellipse_t Ellipse(vec2_t Position, vec2_t Size)
    {
        return gInternal::Ellipse_t(Devicecontext, Position, Size);
    }
    gInternal::Path_t Path(vec2_t Startvec, vec2_t Endvec)
    {
        return gInternal::Path_t(Devicecontext, Startvec, Endvec);
    }
    gInternal::Arc_t Arc(vec2_t Position, vec2_t Angles, uint8_t Rounding)
    {
        return gInternal::Arc_t(Devicecontext, Position, Angles, Rounding);
    }
    gInternal::Quad_t Quad(vec2_t Position, vec2_t Size, uint8_t Rounding = 0)
    {
        return gInternal::Quad_t(Devicecontext, Position, Size, Rounding);
    }
    gInternal::Mesh_t Mesh(std::vector<POINT> Points, std::vector<COLORREF> Colors)
    {
        return gInternal::Mesh_t(Devicecontext, Points, Colors);
    }
    gInternal::Text_t Text(COLORREF Textcolor) { return gInternal::Text_t(Devicecontext, Textcolor); }
    gInternal::Path_t Path(std::vector<vec2_t> Points) { return gInternal::Path_t(Devicecontext, Points); }
};
#pragma pack(pop)

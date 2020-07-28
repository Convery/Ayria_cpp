/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-07-22
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>
#include <numbers>

#pragma pack(push, 1)
using point2_t = struct { int16_t x, y; };
using point3_t = struct { int16_t x, y, z; };
using point4_t = struct { int16_t x, y, z, w; };
using Surface_t = struct { point3_t Position; point2_t Size; HDC Devicecontext; HDC Mask; };

struct Texture2D
{
    HBITMAP Internalbitmap;
    point2_t Internalsize;

    // Windows wants to use BGR for bitmaps, probably some Win16 reasoning.
    static void RGBA_SwapRB(size_t Size, void *Data);
    static void RGB_SwapRB(size_t Size, void *Data);

    explicit Texture2D(std::string_view Filepath);
    explicit Texture2D(point2_t Dimensions, size_t Size, const void *Data) : Internalsize(Dimensions)
    {
        assert(Dimensions.x); assert(Dimensions.y); assert(Size); assert(Data);
        const size_t Pixelsize{ Size / (Dimensions.x * Dimensions.y) };
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

        // Sync.
        GdiFlush();

        // Re-associate the memory with our bitmap.
        SetDIBits(NULL, Internalbitmap, 0, static_cast<UINT>(Dimensions.y), DIBPixels, &BMI, DIB_RGB_COLORS);
    }
};

namespace gInternal
{
    constexpr COLORREF toColorref(uint8_t R, uint8_t G, uint8_t B, uint8_t A = 0xFF)
    {
        return R | (G << 8U) | (B << 16U) | (A << 24U);
    }

    struct Arc_t
    {
        point2_t Focalpoint0, Focalpoint1;
        point4_t Boundingbox;
        HDC Devicecontext;

        void Solid(COLORREF Color);
        void Solid(Texture2D Texture);
        void Outline(uint8_t Linewidth, COLORREF Color);
        void Filled(uint8_t Linewidth, COLORREF Outline, COLORREF Background);
        void Filled(uint8_t Linewidth, COLORREF Outline, Texture2D Background);

        explicit Arc_t(HDC Context, point2_t Position, point2_t Angles, uint8_t Rounding) : Devicecontext(Context),
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
        point4_t Dimensions;
        HDC Devicecontext;

        void Solid(COLORREF Color);
        void Solid(Texture2D Texture);
        void Outline(uint8_t Linewidth, COLORREF Color);
        void Filled(uint8_t Linewidth, COLORREF Outline, COLORREF Background);
        void Filled(uint8_t Linewidth, COLORREF Outline, Texture2D Background);

        explicit Ellipse_t(HDC Context, point2_t Position, point2_t Size) : Devicecontext(Context),
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

            Vertexcount = Points.size();
            Indexcount = Points.size() / 3;
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

        explicit Path_t(HDC Context, std::vector<point2_t> vPoints) : Devicecontext(Context)
        {
            Points = std::make_unique<POINT[]>(vPoints.size());
            Pointcount = vPoints.size();

            for (size_t i = 0; i < vPoints.size(); ++i)
            {
                Points[i].x = vPoints[i].x;
                Points[i].y = vPoints[i].y;
            }
        }
        explicit Path_t(HDC Context, point2_t Startpoint, point2_t Endpoint) : Devicecontext(Context)
        {
            Points = std::make_unique<POINT[]>(2);
            Pointcount = 2;

            Points[0] = { Startpoint.x, Startpoint.y };
            Points[1] = { Endpoint.x, Endpoint.y };
        }
    };
    struct Quad_t
    {
        point4_t Dimensions;
        HDC Devicecontext;
        uint8_t Radius;

        void Solid(COLORREF Color);
        void Solid(Texture2D Texture);
        void Outline(uint8_t Linewidth, COLORREF Color);
        void Filled(uint8_t Linewidth, COLORREF Outline, COLORREF Background);
        void Filled(uint8_t Linewidth, COLORREF Outline, Texture2D Background);

        explicit Quad_t(HDC Context, point2_t Position, point2_t Size, uint8_t Rounding) : Devicecontext(Context),
            Radius(Rounding), Dimensions{ Position.x, Position.y, Position.x + Size.x, Position.y + Size.y } {}
    };
    struct Text_t
    {
        HDC Devicecontext;
        COLORREF Color;
        HFONT Font;

        point2_t getTextsize(std::wstring_view String);
        void Transparent(point2_t Position, std::wstring_view String);
        void Opaque(point2_t Position, std::wstring_view String, COLORREF Background);

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
    explicit Graphics(HDC Context, point4_t Boundingbox) : Devicecontext(Context)
    {
        SelectClipRgn(Devicecontext, CreateRectRgn(Boundingbox.x, Boundingbox.y, Boundingbox.z, Boundingbox.w));
    }

    // Create a mask for "transparency".
    static HDC Createmask(HDC Devicecontext, point2_t Size, COLORREF Transparancykey);

    // Find windows not associated with our threads.
    static std::vector<std::pair<HWND, point2_t>> Findwindows();

    gInternal::Text_t Text(COLORREF Textcolor, HFONT Fonthandle)
    {
        return gInternal::Text_t(Devicecontext, Textcolor, Fonthandle);
    }
    gInternal::Ellipse_t Ellipse(point2_t Position, point2_t Size)
    {
        return gInternal::Ellipse_t(Devicecontext, Position, Size);
    }
    gInternal::Path_t Path(point2_t Startpoint, point2_t Endpoint)
    {
        return gInternal::Path_t(Devicecontext, Startpoint, Endpoint);
    }
    gInternal::Arc_t Arc(point2_t Position, point2_t Angles, uint8_t Rounding)
    {
        return gInternal::Arc_t(Devicecontext, Position, Angles, Rounding);
    }
    gInternal::Quad_t Quad(point2_t Position, point2_t Size, uint8_t Rounding = 0)
    {
        return gInternal::Quad_t(Devicecontext, Position, Size, Rounding);
    }
    gInternal::Mesh_t Mesh(std::vector<POINT> Points, std::vector<COLORREF> Colors)
    {
        return gInternal::Mesh_t(Devicecontext, Points, Colors);
    }
    gInternal::Text_t Text(COLORREF Textcolor) { return gInternal::Text_t(Devicecontext, Textcolor); }
    gInternal::Path_t Path(std::vector<point2_t> Points) { return gInternal::Path_t(Devicecontext, Points); }
};

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
using Eventcallback_t = std::function<void(const Event_t &)>;
using Eventpack_t = std::pair<Eventflags_t, Eventcallback_t>;

struct Overlay_t
{
    std::vector<Eventpack_t> Eventcallbacks{};
    std::vector<Surface_t> Surfaces{};
    point2_t Position, Size;
    HDC Devicecontext;
    HWND Windowhandle;

    void doFrame();
    void Updatesurface(Surface_t &Template);
    Surface_t Createsurface(point2_t Size, point3_t Position, Eventpack_t Eventinfo);
    explicit Overlay_t(point2_t Windowsize, point2_t Windowposition = {});
};
#pragma pack(pop)

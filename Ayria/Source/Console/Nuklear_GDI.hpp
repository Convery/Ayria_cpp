/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-02-27
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_FIXED_TYPES
#define NK_ZERO_COMMAND_MEMORY
#pragma warning(push, 0)
#include <nuklear.h>
#pragma warning(pop)

namespace NK_GDI
{
    struct Font_t
    {
        HDC Devicecontext;
        HFONT Fonthandle;
        LONG Fontheight;
    };

    // Uses Arial as default font.
    nk_context *Initialize(HDC Windowdevice);

    // Clear the buffer to white by default as we tend to chroma-key on it.
    void onRender(struct nk_color Clearcolor = { 0xFF, 0xFF, 0xFF, 0xFF });

    // Returns whether or not to sink the event.
    bool onEvent(HWND, UINT, WPARAM, LPARAM);

    namespace Fonts
    {
        struct nk_user_font Createfont(const char *Name, int32_t Fontsize);
        struct nk_user_font Createfont(const char *Name, int32_t Fontsize, void *Fontdata, uint32_t Datasize);
    }
    namespace Images
    {
        template <bool isRGB = true> // NOTE(tcn): Windows wants to use BGR for bitmaps, probably some Win16 reasoning.
        std::unique_ptr<struct nk_image> Createimage(const uint8_t *Pixelbuffer, struct nk_vec2i Size);
        void Deleteimage(std::unique_ptr<struct nk_image> &&Image);
    }
    namespace Internal
    {
        // Callbacks for nk_command_type.
        template <typename CMD> void RenderCMD(const CMD *Command);
        constexpr COLORREF toColor(const struct nk_color &Color);
    }
}

// Let's use Nuklears include style.
#ifdef NK_IMPLEMENTATION
namespace NK_GDI
{
    constexpr size_t Bitsperpixel = 24;
    constexpr size_t Pixelsize = 3;
    static struct
    {
        HDC Memorydevice;
        HDC Windowdevice;
        struct nk_vec2i Size;
        struct nk_context NK;
    } Context{};

    // Use UTF-8 for everything, just to be safe.
    inline std::wstring getWidestring(const char *Input, const size_t Length)
    {
        const auto Size = MultiByteToWideChar(CP_UTF8, 0, Input, Length, NULL, 0);
        auto Buffer = (wchar_t *)alloca(Size * sizeof(wchar_t) + sizeof(wchar_t));
        MultiByteToWideChar(CP_UTF8, 0, Input, Length, Buffer, Size);
        return std::wstring(Buffer, Size);
    }
    inline std::wstring getWidestring(const std::string &Input)
    {
        return getWidestring(Input.c_str(), Input.size());
    }

    namespace Fonts
    {
        inline float getTextwidth(nk_handle Handle, float, const char *Text, int Length)
        {
            assert(Handle.ptr); assert(Text);

            SIZE Textsize;
            const auto Font = (Font_t *)Handle.ptr;
            const auto String = getWidestring(Text, Length);

            if (GetTextExtentPoint32W(Font->Devicecontext, String.c_str(), String.size(), &Textsize))
                return float(Textsize.cx);

            return -1.0f;
        }

        inline struct nk_user_font Createfont(const char *Name, int32_t Fontsize)
        {
            auto Font = new Font_t();
            struct nk_user_font NK;
            TEXTMETRICW Fontmetric;

            Font->Devicecontext = CreateCompatibleDC(NULL);
            Font->Fonthandle = CreateFontA(Fontsize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, Name);

            SelectObject(Font->Devicecontext, Font->Fonthandle);
            GetTextMetricsW(Font->Devicecontext, &Fontmetric);
            Font->Fontheight = Fontmetric.tmHeight;
            NK.height = float(Font->Fontheight);
            NK.userdata = nk_handle_ptr(Font);
            NK.width = getTextwidth;

            return NK;
        }
        inline struct nk_user_font Createfont(const char *Name, int32_t Fontsize, void *Fontdata, uint32_t Datasize)
        {
            DWORD Installedfonts{};
            (void)AddFontMemResourceEx(Fontdata, Datasize, NULL, &Installedfonts);
            return Createfont(Name, Fontsize);
        }
    }
    namespace Images
    {
        template <bool isRGB>   // NOTE(tcn): Windows wants to use BGR for bitmaps, probably some Win16 reasoning.
        std::unique_ptr<struct nk_image> Createimage(const uint8_t *Pixelbuffer, const struct nk_vec2i Size)
        {
            assert(Size.x); assert(Size.y); assert(Pixelbuffer);
            auto Image = std::make_unique<struct nk_image>();
            BITMAPINFO BMI{ sizeof(BITMAPINFOHEADER) };
            uint8_t *DIBPixels{};

            // Windows likes to draw bitmaps upside down, so negative height.
            BMI.bmiHeader.biSizeImage = Size.y * Size.x * Pixelsize;
            BMI.bmiHeader.biBitCount = Bitsperpixel;
            BMI.bmiHeader.biHeight = (-1) * Size.y;
            BMI.bmiHeader.biCompression = BI_RGB;
            BMI.bmiHeader.biWidth = Size.x;
            BMI.bmiHeader.biPlanes = 1;

            // Region is a nk_recti {x0, y0, w, h }
            Image->region[2] = Size.x;
            Image->region[3] = Size.y;
            Image->w = Size.x;
            Image->h = Size.y;

            // Allocate the bitmap in system-memory.
            auto Bitmap = CreateDIBSection(NULL, &BMI, DIB_RGB_COLORS, (void **)&DIBPixels, NULL, 0);
            std::memcpy(Bitmap, Pixelbuffer, Size.x * Size.y * Pixelsize);

            // Swap R and B channels.
            if constexpr (isRGB)
            {
                // TODO(tcn): Maybe some kind of SSE optimization here?
                for (size_t i = 0; i < Size.x * Size.y; i += Pixelsize)
                {
                    std::swap(DIBPixels + i, DIBPixels + i + 2);
                }
            }

            // Re-associate the memory with our bitmap.
            SetDIBits(NULL, Bitmap, 0, Size.y, DIBPixels, &BMI, DIB_RGB_COLORS);
            Image->handle.ptr = Bitmap;
            return Image;
        }
        void Deleteimage(std::unique_ptr<struct nk_image> &&Image)
        {
            assert(Image->handle.ptr);
            DeleteObject(Image->handle.ptr);
        }
    }
    namespace Internal
    {
        constexpr COLORREF toColor(const struct nk_color &Color)
        {
            return Color.r | (Color.g << 8) | (Color.b << 16);
        }

        template </*NK_COMMAND_TEXT*/> void RenderCMD(const nk_command_text *Command)
        {
            assert(Context.Memorydevice); assert(Command);
            assert(Command->string); assert(Command->font);

            const auto Text = getWidestring(Command->string, Command->length);
            SetTextColor(Context.Memorydevice, toColor(Command->foreground));
            SetBkColor(Context.Memorydevice, toColor(Command->background));

            SelectObject(Context.Memorydevice, ((Font_t *)Command->font->userdata.ptr)->Fonthandle);
            ExtTextOutW(Context.Memorydevice, Command->x, Command->y, ETO_OPAQUE, NULL, Text.c_str(), Text.size(), NULL);
        }
        template </*NK_COMMAND_RECT*/> void RenderCMD(const nk_command_rect *Command)
        {
            assert(Context.Memorydevice); assert(Command);
            HPEN Pen{};

            // Create a custom pen if needed.
            if (Command->line_thickness != 1)
            {
                Pen = CreatePen(PS_SOLID, Command->line_thickness, toColor(Command->color));
                SelectObject(Context.Memorydevice, Pen);
            }
            else SetDCPenColor(Context.Memorydevice, toColor(Command->color));

            // No fill.
            auto Brush = SelectObject(Context.Memorydevice, GetStockObject(NULL_BRUSH));
            if (Command->rounding == 0)
                Rectangle(Context.Memorydevice, Command->x, Command->y, Command->x + Command->w, Command->y + Command->h);
            else
                RoundRect(Context.Memorydevice, Command->x, Command->y, Command->x + Command->w, Command->y + Command->h, Command->rounding, Command->rounding);

            // Restore.
            SelectObject(Context.Memorydevice, Brush);
            if (Pen)
            {
                SelectObject(Context.Memorydevice, GetStockObject(DC_PEN));
                DeleteObject(Pen);
            }
        }
        template </*NK_COMMAND_LINE*/> void RenderCMD(const nk_command_line *Command)
        {
            assert(Context.Memorydevice); assert(Command);
            HPEN Pen{};

            // Create a custom pen if needed.
            if (Command->line_thickness != 1)
            {
                Pen = CreatePen(PS_SOLID, Command->line_thickness, toColor(Command->color));
                SelectObject(Context.Memorydevice, Pen);
            }
            else SetDCPenColor(Context.Memorydevice, toColor(Command->color));

            // Draw the line.
            MoveToEx(Context.Memorydevice, Command->begin.x, Command->begin.y, NULL);
            LineTo(Context.Memorydevice, Command->end.x, Command->end.y);

            // Restore.
            if (Pen)
            {
                SelectObject(Context.Memorydevice, GetStockObject(DC_PEN));
                DeleteObject(Pen);
            }
        }
        template </*NK_COMMAND_IMAGE*/> void RenderCMD(const nk_command_image *Command)
        {
            assert(Context.Memorydevice); assert(Command);

            BITMAP Bitmap{};
            auto Device = CreateCompatibleDC(Context.Memorydevice);
            GetObjectA(Command->img.handle.ptr, sizeof(BITMAP), &Bitmap);

            SelectObject(Context.Memorydevice, Command->img.handle.ptr);
            StretchBlt(Context.Memorydevice, Command->x, Command->y, Command->w, Command->h,
                Device, 0, 0, Bitmap.bmWidth, Bitmap.bmHeight, SRCCOPY);
            DeleteDC(Device);
        }
        template </*NK_COMMAND_CIRCLE*/> void RenderCMD(const nk_command_circle *Command)
        {
            assert(Context.Memorydevice); assert(Command);
            HPEN Pen{};

            // Create a custom pen if needed.
            if (Command->line_thickness != 1)
            {
                Pen = CreatePen(PS_SOLID, Command->line_thickness, toColor(Command->color));
                SelectObject(Context.Memorydevice, Pen);
            }
            else SetDCPenColor(Context.Memorydevice, toColor(Command->color));

            SetDCBrushColor(Context.Memorydevice, OPAQUE);
            Ellipse(Context.Memorydevice, Command->x, Command->y, Command->x + Command->w, Command->y + Command->h);

            // Restore.
            if (Pen)
            {
                SelectObject(Context.Memorydevice, GetStockObject(DC_PEN));
                DeleteObject(Pen);
            }
        }
        template </*NK_COMMAND_SCISSOR*/> void RenderCMD(const nk_command_scissor *Command)
        {
            assert(Context.Memorydevice); assert(Command);

            SelectClipRgn(Context.Memorydevice, NULL);
            IntersectClipRect(Context.Memorydevice, Command->x, Command->y,
                Command->x + Command->w + 1, Command->y + Command->h + 1);
        }
        template </*NK_COMMAND_POLYGON*/> void RenderCMD(const nk_command_polygon *Command)
        {
            assert(Context.Memorydevice); assert(Command); assert(Command->point_count);

            auto Points = (POINT *)alloca(Command->point_count * sizeof(POINT));
            for (int i = 0; i < Command->point_count; ++i)
            {
                Points[i] = { Command->points[i].x, Command->points[i].y };
            }
            HPEN Pen{};

            // Create a custom pen if needed.
            if (Command->line_thickness != 1)
            {
                Pen = CreatePen(PS_SOLID, Command->line_thickness, toColor(Command->color));
                SelectObject(Context.Memorydevice, Pen);
            }
            else SetDCPenColor(Context.Memorydevice, toColor(Command->color));

            Polyline(Context.Memorydevice, Points, Command->point_count);

            // Restore.
            if (Pen)
            {
                SelectObject(Context.Memorydevice, GetStockObject(DC_PEN));
                DeleteObject(Pen);
            }
        }
        template </*NK_COMMAND_POLYLINE*/> void RenderCMD(const nk_command_polyline *Command)
        {
            assert(Context.Memorydevice); assert(Command); assert(Command->point_count);

            auto Points = (POINT *)alloca(Command->point_count * sizeof(POINT));
            for (int i = 0; i < Command->point_count; ++i)
            {
                Points[i] = { Command->points[i].x, Command->points[i].y };
            }
            HPEN Pen{};

            // Create a custom pen if needed.
            if (Command->line_thickness != 1)
            {
                Pen = CreatePen(PS_SOLID, Command->line_thickness, toColor(Command->color));
                SelectObject(Context.Memorydevice, Pen);
            }
            else SetDCPenColor(Context.Memorydevice, toColor(Command->color));

            Polyline(Context.Memorydevice, Points, Command->point_count);

            // Restore.
            if (Pen)
            {
                SelectObject(Context.Memorydevice, GetStockObject(DC_PEN));
                DeleteObject(Pen);
            }
        }
        template </*NK_COMMAND_TRIANGLE*/> void RenderCMD(const nk_command_triangle *Command)
        {
            assert(Context.Memorydevice); assert(Command);
            POINT Points[4] = { {Command->a.x, Command->a.y}, {Command->b.x, Command->b.y},
                               {Command->c.x, Command->c.y}, {Command->a.x, Command->a.y} };
            HPEN Pen{};

            // Create a custom pen if needed.
            if (Command->line_thickness != 1)
            {
                Pen = CreatePen(PS_SOLID, Command->line_thickness, toColor(Command->color));
                SelectObject(Context.Memorydevice, Pen);
            }
            else SetDCPenColor(Context.Memorydevice, toColor(Command->color));

            Polyline(Context.Memorydevice, Points, 4);

            // Restore.
            if (Pen)
            {
                SelectObject(Context.Memorydevice, GetStockObject(DC_PEN));
                DeleteObject(Pen);
            }
        }
        template </*NK_COMMAND_RECT_FILLED*/> void RenderCMD(const nk_command_rect_filled *Command)
        {
            assert(Context.Memorydevice); assert(Command);

            // Common case.
            if (Command->rounding == 0)
            {
                // TODO(tcn): Benchmark the different ways to paint the rect.
                const RECT Area = { Command->x, Command->y, Command->x + Command->w, Command->y + Command->h };
                SetBkColor(Context.Memorydevice, Internal::toColor(Command->color));
                ExtTextOutW(Context.Memorydevice, 0, 0, ETO_OPAQUE, &Area, NULL, 0, NULL);
            }
            else
            {
                SetDCPenColor(Context.Memorydevice, Internal::toColor(Command->color));
                SetDCBrushColor(Context.Memorydevice, Internal::toColor(Command->color));
                RoundRect(Context.Memorydevice, Command->x, Command->y, Command->x + Command->w,
                    Command->y + Command->h, Command->rounding, Command->rounding);
            }
        }
        template </*NK_COMMAND_CIRCLE_FILLED*/> void RenderCMD(const nk_command_circle_filled *Command)
        {
            assert(Context.Memorydevice); assert(Command);
            SetDCPenColor(Context.Memorydevice, toColor(Command->color));
            SetDCBrushColor(Context.Memorydevice, toColor(Command->color));
            Ellipse(Context.Memorydevice, Command->x, Command->y, Command->x + Command->w, Command->y + Command->h);
        }
        template </*NK_COMMAND_POLYGON_FILLED*/> void RenderCMD(const nk_command_polygon_filled *Command)
        {
            assert(Context.Memorydevice); assert(Command); assert(Command->point_count);

            auto Points = (POINT *)alloca(Command->point_count * sizeof(POINT));
            for (int i = 0; i < Command->point_count; ++i)
            {
                Points[i] = { Command->points[i].x, Command->points[i].y };
            }

            SetDCBrushColor(Context.Memorydevice, toColor(Command->color));
            SetDCPenColor(Context.Memorydevice, toColor(Command->color));
            Polygon(Context.Memorydevice, Points, Command->point_count);
        }
        template </*NK_COMMAND_TRIANGLE_FILLED*/> void RenderCMD(const nk_command_triangle_filled *Command)
        {
            assert(Context.Memorydevice); assert(Command);

            POINT Points[3] = { {Command->a.x, Command->a.y}, {Command->b.x, Command->b.y}, {Command->c.x, Command->c.y} };
            SetDCPenColor(Context.Memorydevice, toColor(Command->color));
            SetDCBrushColor(Context.Memorydevice, toColor(Command->color));
            Polygon(Context.Memorydevice, Points, 3);
        }
        template </*NK_COMMAND_RECT_MULTI_COLOR*/> void RenderCMD(const nk_command_rect_multi_color *Command)
        {
            assert(Context.Memorydevice); assert(Command);
            auto addColor = [](TRIVERTEX &Vertex, const struct nk_color &Color)
            {
                Vertex.Red = Color.r << 8;
                Vertex.Green = Color.g << 8;
                Vertex.Blue = Color.b << 8;
                Vertex.Alpha = 0xFF << 8;
            };

            // GDI only takes two vertices for a rect.
            GRADIENT_RECT Mesh{ 0, 1 };
            TRIVERTEX Vertices[2]{};
            Vertices[0].x = Command->x;
            Vertices[0].y = Command->y;
            Vertices[1].x = Command->x + Command->w;
            Vertices[1].y = Command->y + Command->h;

            // Horizontal gradient.
            if (0 == std::memcmp(&Command->top, &Command->bottom, sizeof(nk_color)))
            {
                addColor(Vertices[0], Command->left);
                addColor(Vertices[1], Command->right);
                GradientFill(Context.Memorydevice, Vertices, 2, &Mesh, 1, GRADIENT_FILL_RECT_H);
            }

            // Vertical gradient.
            else if (0 == std::memcmp(&Command->left, &Command->right, sizeof(nk_color)))
            {
                addColor(Vertices[0], Command->top);
                addColor(Vertices[1], Command->bottom);
                GradientFill(Context.Memorydevice, Vertices, 2, &Mesh, 1, GRADIENT_FILL_RECT_V);
            }

            // Angular gradient.
            else
            {
                /*
                TODO(tcn): Up vertices to a multiple of 4, do maths with GRADIENT_FILL_TRIANGLE..
                */
            }

            // TODO(tcn): AlphaBlend?
        }

        /*
        template <> void RenderCMD(const nk_command_scissor *Command)
        {
            assert(Context.Memorydevice); assert(Command);
        }
        */
    }

    // Uses Arial as default font.
    inline nk_context *Initialize(HDC Windowdevice)
    {
        // Arial should be available on every system.
        static auto Systemfont = Fonts::Createfont("Arial", 14);

        auto Bitmap = CreateCompatibleBitmap(Windowdevice, 0, 0);
        Context.Memorydevice = CreateCompatibleDC(Windowdevice);
        SelectObject(Context.Memorydevice, Bitmap);

        nk_init_default(&Context.NK, &Systemfont);
        Context.Windowdevice = Windowdevice;
        return &Context.NK;
    }

    // Returns whether or not to sink the event.
    inline bool onEvent(HWND Windowhandle, UINT Message, WPARAM wParam, LPARAM lParam)
    {
        switch (Message)
        {
            // Window-control.
            case WM_SIZE:
            {
                const uint16_t Width = LOWORD(lParam);
                const uint16_t Height = HIWORD(lParam);

                if (Context.Size.x != Width || Context.Size.y != Height)
                {
                    auto Bitmap = CreateCompatibleBitmap(Context.Windowdevice, Width, Height);
                    auto Old = SelectObject(Context.Memorydevice, Bitmap);
                    Context.Size.y = Height;
                    Context.Size.x = Width;
                    DeleteObject(Old);
                }

                return false;
            }
            case WM_PAINT:
            {
                PAINTSTRUCT Updateinformation{};

                auto Devicecontext = BeginPaint((HWND)Windowhandle, &Updateinformation);
                BitBlt(Devicecontext, 0, 0, Context.Size.x, Context.Size.y, Context.Memorydevice, 0, 0, SRCCOPY);
                EndPaint((HWND)Windowhandle, &Updateinformation);

                return true;
            }

            // Special keys.
            case WM_KEYDOWN:
            case WM_KEYUP:
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            {
                // Modifiers.
                const bool Down = !((lParam >> 31) & 1);
                const bool Shift = GetKeyState(VK_SHIFT) & (1 << 15);
                const bool Ctrl = GetKeyState(VK_CONTROL) & (1 << 15);

                switch (wParam)
                {
                    case VK_SHIFT:
                    case VK_LSHIFT:
                    case VK_RSHIFT:
                        nk_input_key(&Context.NK, NK_KEY_SHIFT, Down);
                        return true;

                    case VK_DELETE:
                        nk_input_key(&Context.NK, NK_KEY_DEL, Down);
                        return true;

                    case VK_RETURN:
                        nk_input_key(&Context.NK, NK_KEY_ENTER, Down);
                        return true;

                    case VK_TAB:
                        nk_input_key(&Context.NK, NK_KEY_TAB, Down);
                        return true;

                    case VK_LEFT:
                        if (!Ctrl) nk_input_key(&Context.NK, NK_KEY_LEFT, Down);
                        else nk_input_key(&Context.NK, NK_KEY_TEXT_WORD_LEFT, Down);
                        return true;

                    case VK_RIGHT:
                        if (!Ctrl) nk_input_key(&Context.NK, NK_KEY_RIGHT, Down);
                        else nk_input_key(&Context.NK, NK_KEY_TEXT_WORD_RIGHT, Down);
                        return true;

                    case VK_BACK:
                        nk_input_key(&Context.NK, NK_KEY_BACKSPACE, Down);
                        return true;

                    case VK_HOME:
                        nk_input_key(&Context.NK, NK_KEY_TEXT_START, Down);
                        nk_input_key(&Context.NK, NK_KEY_SCROLL_START, Down);
                        return true;

                    case VK_END:
                        nk_input_key(&Context.NK, NK_KEY_TEXT_END, Down);
                        nk_input_key(&Context.NK, NK_KEY_SCROLL_END, Down);
                        return true;

                    case VK_NEXT:
                        nk_input_key(&Context.NK, NK_KEY_SCROLL_DOWN, Down);
                        return true;

                    case VK_PRIOR:
                        nk_input_key(&Context.NK, NK_KEY_SCROLL_UP, Down);
                        return true;

                    case 'C':
                        if (Ctrl)
                        {
                            nk_input_key(&Context.NK, NK_KEY_COPY, Down);
                            return true;
                        }
                        break;

                    case 'V':
                        if (Ctrl)
                        {
                            nk_input_key(&Context.NK, NK_KEY_PASTE, Down);
                            return true;
                        }
                        break;

                    case 'X':
                        if (Ctrl)
                        {
                            nk_input_key(&Context.NK, NK_KEY_CUT, Down);
                            return true;
                        }
                        break;

                    case 'Z':
                        if (Ctrl)
                        {
                            if (Shift) nk_input_key(&Context.NK, NK_KEY_TEXT_REDO, Down);
                            else nk_input_key(&Context.NK, NK_KEY_TEXT_UNDO, Down);
                            return true;
                        }
                        break;

                    case 'R':
                        if (Ctrl)
                        {
                            nk_input_key(&Context.NK, NK_KEY_TEXT_REDO, Down);
                            return true;
                        }
                        break;
                }

                return false;
            }

            // Keyboard input.
            case WM_CHAR:
            {
                if (wParam >= 32)
                {
                    nk_input_unicode(&Context.NK, (nk_rune)wParam);
                    return true;
                }
                break;
            }

            // Mouse input.
            case WM_LBUTTONDOWN:
            {
                nk_input_button(&Context.NK, NK_BUTTON_LEFT, (short)LOWORD(lParam), (short)HIWORD(lParam), 1);
                SetCapture(Windowhandle);
                return true;
            }
            case WM_LBUTTONUP:
            {
                nk_input_button(&Context.NK, NK_BUTTON_DOUBLE, (short)LOWORD(lParam), (short)HIWORD(lParam), 0);
                nk_input_button(&Context.NK, NK_BUTTON_LEFT, (short)LOWORD(lParam), (short)HIWORD(lParam), 0);
                ReleaseCapture();
                return true;
            }
            case WM_RBUTTONDOWN:
            {
                nk_input_button(&Context.NK, NK_BUTTON_RIGHT, (short)LOWORD(lParam), (short)HIWORD(lParam), 1);
                SetCapture(Windowhandle);
                return true;
            }
            case WM_RBUTTONUP:
            {
                nk_input_button(&Context.NK, NK_BUTTON_RIGHT, (short)LOWORD(lParam), (short)HIWORD(lParam), 0);
                ReleaseCapture();
                return true;
            }
            case WM_MBUTTONDOWN:
            {
                nk_input_button(&Context.NK, NK_BUTTON_MIDDLE, (short)LOWORD(lParam), (short)HIWORD(lParam), 1);
                SetCapture(Windowhandle);
                return true;
            }
            case WM_MBUTTONUP:
            {
                nk_input_button(&Context.NK, NK_BUTTON_MIDDLE, (short)LOWORD(lParam), (short)HIWORD(lParam), 0);
                ReleaseCapture();
                return true;
            }
            case WM_MOUSEWHEEL:
            {
                nk_input_scroll(&Context.NK, nk_vec2(0, (float)(short)HIWORD(wParam) / WHEEL_DELTA));
                return true;
            }
            case WM_MOUSEMOVE:
            {
                nk_input_motion(&Context.NK, (short)LOWORD(lParam), (short)HIWORD(lParam));
                return true;
            }
            case WM_LBUTTONDBLCLK:
            {
                nk_input_button(&Context.NK, NK_BUTTON_DOUBLE, (short)LOWORD(lParam), (short)HIWORD(lParam), 1);
                return true;
            }
        }

        return false;
    }

    // Clear the buffer to white by default as we tend to chroma-key on it.
    inline void Render(struct nk_color Clearcolor = { 0xFF, 0xFF, 0xFF, 0xFF })
    {
        // Ensure that the context is clean.
        SelectObject(Context.Memorydevice, GetStockObject(DC_PEN));
        SelectObject(Context.Memorydevice, GetStockObject(DC_BRUSH));

        // Clear the device.
        {
            // TODO(tcn): Benchmark the different ways to clear.
            const RECT Screen = { 0, 0, Context.Size.x, Context.Size.y };
            SetBkColor(Context.Memorydevice, Internal::toColor(Clearcolor));
            ExtTextOutW(Context.Memorydevice, 0, 0, ETO_OPAQUE, &Screen, NULL, 0, NULL);
        }

        // Iterate over the render commands.
        auto Command = nk__begin(&Context.NK);
        while (Command)
        {
            switch (Command->type)
            {
                #define Case(x, y) case x: Internal::RenderCMD((y *)Command); break;
                Case(NK_COMMAND_RECT_MULTI_COLOR, nk_command_rect_multi_color);
                Case(NK_COMMAND_TRIANGLE_FILLED, nk_command_triangle_filled);
                Case(NK_COMMAND_POLYGON_FILLED, nk_command_polygon_filled);
                Case(NK_COMMAND_CIRCLE_FILLED, nk_command_circle_filled);
                Case(NK_COMMAND_RECT_FILLED, nk_command_rect_filled);
                // Case(NK_COMMAND_ARC_FILLED, nk_command_arc_filled);
                Case(NK_COMMAND_POLYLINE, nk_command_polyline);
                Case(NK_COMMAND_TRIANGLE, nk_command_triangle);
                Case(NK_COMMAND_POLYGON, nk_command_polygon);
                Case(NK_COMMAND_SCISSOR, nk_command_scissor);
                Case(NK_COMMAND_CIRCLE, nk_command_circle);
                // Case(NK_COMMAND_CUSTOM, nk_command_custom);
                // Case(NK_COMMAND_CURVE, nk_command_curve);
                Case(NK_COMMAND_IMAGE, nk_command_image);
                Case(NK_COMMAND_TEXT, nk_command_text);
                Case(NK_COMMAND_RECT, nk_command_rect);
                Case(NK_COMMAND_LINE, nk_command_line);
                // Case(NK_COMMAND_ARC, nk_command_arc);
                case NK_COMMAND_NOP:
                default: break;
                    #undef Case
            }

            // TODO(tcn): Add a custom allocator to reduce the impact of this linked list.
            Command = nk__next(&Context.NK, Command);
        }

        // Copy to the screen buffer.
        BitBlt(Context.Windowdevice, 0, 0, Context.Size.x, Context.Size.y, Context.Memorydevice, 0, 0, SRCCOPY);

        // Cleanup.
        nk_clear(&Context.NK);
    }
}
#endif

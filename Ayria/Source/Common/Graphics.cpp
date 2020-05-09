/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-05-07
    License: MIT
*/

#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_FIXED_TYPES
#define NK_ZERO_COMMAND_MEMORY
#define NK_IMPLEMENTATION
#include "../Global.hpp"

namespace Graphics
{
    using vec2i = struct { int x, y; };
    using vec4i = struct { int x, y, z, w; };

    #pragma pack(push, 1)
    struct
    {
        nk_context Context;
        void *Windowhandle;
        HDC Memorydevice;
        HDC Windowdevice;
        vec4i Drawarea;
        bool isDirty;
        vec2i Size;

    } Global{};
    #pragma pack(pop)

    constexpr nk_color Clearcolor = { 0xFF, 0xFF, 0xFF, 0xFF };
    constexpr size_t Bitsperpixel = 24;
    constexpr size_t Pixelsize = 3;

    // Helpers for Nuklear.
    namespace Fonts
    {
        inline float getTextwidth(nk_handle Handle, float, const char *Text, int Length)
        {
            assert(Handle.ptr); assert(Text);

            SIZE Textsize;
            const auto Font = (Font_t *)Handle.ptr;
            const auto String = toWide(Text, Length);

            if (GetTextExtentPoint32W(Font->Devicecontext, String.c_str(), int(String.size()), &Textsize))
                return float(Textsize.cx);

            return -1.0f;
        }

        inline struct nk_user_font Createfont(const char *Name, int32_t Fontsize)
        {
            const auto Font = new Font_t();
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
            const auto Bitmap = CreateDIBSection(NULL, &BMI, DIB_RGB_COLORS, (void **)&DIBPixels, NULL, 0);
            std::memcpy(Bitmap, Pixelbuffer, Size.x * Size.y * Pixelsize);

            // Swap R and B channels.
            if constexpr (isRGB)
            {
                // TODO(tcn): Maybe some kind of SSE optimization here?
                for (size_t i = 0; i < Size.x * Size.y; i += Pixelsize)
                {
                    std::swap(DIBPixels[i], DIBPixels[i + 2]);
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

        template <typename CMD> void RenderCMD(const CMD *Command);
        template </*NK_COMMAND_ARC*/> void RenderCMD(const nk_command_arc *Command)
        {
            assert(Global.Memorydevice); assert(Command);
            HPEN Pen{};

            // Create a custom pen if needed.
            if (Command->line_thickness != 1)
            {
                Pen = CreatePen(PS_SOLID, Command->line_thickness, toColor(Command->color));
                SelectObject(Global.Memorydevice, Pen);
            }
            else SetDCPenColor(Global.Memorydevice, toColor(Command->color));

            const float X0 = Command->cx + Command->r * std::cosf((Command->a[0] + Command->a[1]) * NK_PI / 180.0f);
            const float Y0 = Command->cy + Command->r * std::sinf((Command->a[0] + Command->a[1]) * NK_PI / 180.0f);
            const float X1 = Command->cx + Command->r * std::cosf(Command->a[0] * NK_PI / 180.0f);
            const float Y1 = Command->cy + Command->r * std::sinf(Command->a[0] * NK_PI / 180.0f);

            SetArcDirection(Global.Memorydevice, AD_COUNTERCLOCKWISE);
            Arc(Global.Memorydevice, Command->cx - Command->r, Command->cy - Command->r,
                Command->cx + Command->r, Command->cy + Command->r,
                (int)X0, (int)Y0, (int)X1, (int)Y1);

            // Restore.
            if (Pen)
            {
                SelectObject(Global.Memorydevice, GetStockObject(DC_PEN));
                DeleteObject(Pen);
            }
        }
        template </*NK_COMMAND_TEXT*/> void RenderCMD(const nk_command_text *Command)
        {
            assert(Global.Memorydevice); assert(Command);
            assert(Command->string); assert(Command->font);

            const auto Text = toWide(Command->string, Command->length);
            SetTextColor(Global.Memorydevice, toColor(Command->foreground));
            SetBkColor(Global.Memorydevice, toColor(Command->background));

            SelectObject(Global.Memorydevice, ((Font_t *)Command->font->userdata.ptr)->Fonthandle);
            ExtTextOutW(Global.Memorydevice, Command->x, Command->y, ETO_OPAQUE, NULL, Text.c_str(), (UINT)Text.size(), NULL);
        }
        template </*NK_COMMAND_RECT*/> void RenderCMD(const nk_command_rect *Command)
        {
            assert(Global.Memorydevice); assert(Command);
            HPEN Pen{};

            // Create a custom pen if needed.
            if (Command->line_thickness != 1)
            {
                Pen = CreatePen(PS_SOLID, Command->line_thickness, toColor(Command->color));
                SelectObject(Global.Memorydevice, Pen);
            }
            else SetDCPenColor(Global.Memorydevice, toColor(Command->color));

            // No fill.
            const auto Brush = SelectObject(Global.Memorydevice, GetStockObject(NULL_BRUSH));
            if (Command->rounding == 0)
                Rectangle(Global.Memorydevice, Command->x, Command->y, Command->x + Command->w, Command->y + Command->h);
            else
                RoundRect(Global.Memorydevice, Command->x, Command->y, Command->x + Command->w, Command->y + Command->h, Command->rounding, Command->rounding);

            // Restore.
            SelectObject(Global.Memorydevice, Brush);
            if (Pen)
            {
                SelectObject(Global.Memorydevice, GetStockObject(DC_PEN));
                DeleteObject(Pen);
            }
        }
        template </*NK_COMMAND_LINE*/> void RenderCMD(const nk_command_line *Command)
        {
            assert(Global.Memorydevice); assert(Command);
            HPEN Pen{};

            // Create a custom pen if needed.
            if (Command->line_thickness != 1)
            {
                Pen = CreatePen(PS_SOLID, Command->line_thickness, toColor(Command->color));
                SelectObject(Global.Memorydevice, Pen);
            }
            else SetDCPenColor(Global.Memorydevice, toColor(Command->color));

            // Draw the line.
            MoveToEx(Global.Memorydevice, Command->begin.x, Command->begin.y, NULL);
            LineTo(Global.Memorydevice, Command->end.x, Command->end.y);

            // Restore.
            if (Pen)
            {
                SelectObject(Global.Memorydevice, GetStockObject(DC_PEN));
                DeleteObject(Pen);
            }
        }
        template </*NK_COMMAND_CURVE*/> void RenderCMD(const nk_command_curve *Command)
        {
            assert(Global.Memorydevice); assert(Command);
            POINT Points[4] =
            {
                {Command->begin.x, Command->begin.y},
                {Command->ctrl[0].x, Command->ctrl[0].y},
                {Command->ctrl[1].x, Command->ctrl[1].y},
                {Command->end.x, Command->end.y}
            };
            HPEN Pen{};

            // Create a custom pen if needed.
            if (Command->line_thickness != 1)
            {
                Pen = CreatePen(PS_SOLID, Command->line_thickness, toColor(Command->color));
                SelectObject(Global.Memorydevice, Pen);
            }
            else SetDCPenColor(Global.Memorydevice, toColor(Command->color));

            SetDCBrushColor(Global.Memorydevice, OPAQUE);
            PolyBezier(Global.Memorydevice, Points, 4);

            // Restore.
            if (Pen)
            {
                SelectObject(Global.Memorydevice, GetStockObject(DC_PEN));
                DeleteObject(Pen);
            }
        }
        template </*NK_COMMAND_IMAGE*/> void RenderCMD(const nk_command_image *Command)
        {
            assert(Global.Memorydevice); assert(Command);

            BITMAP Bitmap{};
            const auto Device = CreateCompatibleDC(Global.Memorydevice);
            GetObjectA(Command->img.handle.ptr, sizeof(BITMAP), &Bitmap);

            SelectObject(Global.Memorydevice, Command->img.handle.ptr);
            StretchBlt(Global.Memorydevice, Command->x, Command->y, Command->w, Command->h,
                Device, 0, 0, Bitmap.bmWidth, Bitmap.bmHeight, SRCCOPY);
            DeleteDC(Device);
        }
        template </*NK_COMMAND_CIRCLE*/> void RenderCMD(const nk_command_circle *Command)
        {
            assert(Global.Memorydevice); assert(Command);
            HPEN Pen{};

            // Create a custom pen if needed.
            if (Command->line_thickness != 1)
            {
                Pen = CreatePen(PS_SOLID, Command->line_thickness, toColor(Command->color));
                SelectObject(Global.Memorydevice, Pen);
            }
            else SetDCPenColor(Global.Memorydevice, toColor(Command->color));

            SetDCBrushColor(Global.Memorydevice, OPAQUE);
            Ellipse(Global.Memorydevice, Command->x, Command->y, Command->x + Command->w, Command->y + Command->h);

            // Restore.
            if (Pen)
            {
                SelectObject(Global.Memorydevice, GetStockObject(DC_PEN));
                DeleteObject(Pen);
            }
        }
        template </*NK_COMMAND_SCISSOR*/> void RenderCMD(const nk_command_scissor *Command)
        {
            assert(Global.Memorydevice); assert(Command);

            SelectClipRgn(Global.Memorydevice, NULL);
            IntersectClipRect(Global.Memorydevice, Command->x, Command->y,
                Command->x + Command->w + 1, Command->y + Command->h + 1);
        }
        template </*NK_COMMAND_POLYGON*/> void RenderCMD(const nk_command_polygon *Command)
        {
            assert(Global.Memorydevice); assert(Command); assert(Command->point_count);

            const auto Points = (POINT *)alloca(Command->point_count * sizeof(POINT));
            for (int i = 0; i < Command->point_count; ++i)
            {
                Points[i] = { Command->points[i].x, Command->points[i].y };
            }
            HPEN Pen{};

            // Create a custom pen if needed.
            if (Command->line_thickness != 1)
            {
                Pen = CreatePen(PS_SOLID, Command->line_thickness, toColor(Command->color));
                SelectObject(Global.Memorydevice, Pen);
            }
            else SetDCPenColor(Global.Memorydevice, toColor(Command->color));

            Polyline(Global.Memorydevice, Points, Command->point_count);

            // Restore.
            if (Pen)
            {
                SelectObject(Global.Memorydevice, GetStockObject(DC_PEN));
                DeleteObject(Pen);
            }
        }
        template </*NK_COMMAND_POLYLINE*/> void RenderCMD(const nk_command_polyline *Command)
        {
            assert(Global.Memorydevice); assert(Command); assert(Command->point_count);

            const auto Points = (POINT *)alloca(Command->point_count * sizeof(POINT));
            for (int i = 0; i < Command->point_count; ++i)
            {
                Points[i] = { Command->points[i].x, Command->points[i].y };
            }
            HPEN Pen{};

            // Create a custom pen if needed.
            if (Command->line_thickness != 1)
            {
                Pen = CreatePen(PS_SOLID, Command->line_thickness, toColor(Command->color));
                SelectObject(Global.Memorydevice, Pen);
            }
            else SetDCPenColor(Global.Memorydevice, toColor(Command->color));

            Polyline(Global.Memorydevice, Points, Command->point_count);

            // Restore.
            if (Pen)
            {
                SelectObject(Global.Memorydevice, GetStockObject(DC_PEN));
                DeleteObject(Pen);
            }
        }
        template </*NK_COMMAND_TRIANGLE*/> void RenderCMD(const nk_command_triangle *Command)
        {
            assert(Global.Memorydevice); assert(Command);
            POINT Points[4] = { {Command->a.x, Command->a.y}, {Command->b.x, Command->b.y},
                {Command->c.x, Command->c.y}, {Command->a.x, Command->a.y} };
            HPEN Pen{};

            // Create a custom pen if needed.
            if (Command->line_thickness != 1)
            {
                Pen = CreatePen(PS_SOLID, Command->line_thickness, toColor(Command->color));
                SelectObject(Global.Memorydevice, Pen);
            }
            else SetDCPenColor(Global.Memorydevice, toColor(Command->color));

            Polyline(Global.Memorydevice, Points, 4);

            // Restore.
            if (Pen)
            {
                SelectObject(Global.Memorydevice, GetStockObject(DC_PEN));
                DeleteObject(Pen);
            }
        }
        template </*NK_COMMAND_ARC_FILLED*/> void RenderCMD(const nk_command_arc_filled *Command)
        {
            assert(Global.Memorydevice); assert(Command);

            SetDCPenColor(Global.Memorydevice, toColor(Command->color));
            SetDCBrushColor(Global.Memorydevice, toColor(Command->color));

            const float X0 = Command->cx + Command->r * std::cosf((Command->a[0] + Command->a[1]) * NK_PI / 180.0f);
            const float Y0 = Command->cy + Command->r * std::sinf((Command->a[0] + Command->a[1]) * NK_PI / 180.0f);
            const float X1 = Command->cx + Command->r * std::cosf(Command->a[0] * NK_PI / 180.0f);
            const float Y1 = Command->cy + Command->r * std::sinf(Command->a[0] * NK_PI / 180.0f);

            Pie(Global.Memorydevice, Command->cx - Command->r, Command->cy - Command->r,
                Command->cx + Command->r, Command->cy + Command->r,
                (int)X0, (int)Y0, (int)X1, (int)Y1);
        }
        template </*NK_COMMAND_RECT_FILLED*/> void RenderCMD(const nk_command_rect_filled *Command)
        {
            assert(Global.Memorydevice); assert(Command);

            // Common case.
            if (Command->rounding == 0)
            {
                // TODO(tcn): Benchmark the different ways to paint the rect.
                const RECT Area = { Command->x, Command->y, Command->x + Command->w, Command->y + Command->h };
                SetBkColor(Global.Memorydevice, Internal::toColor(Command->color));
                ExtTextOutW(Global.Memorydevice, 0, 0, ETO_OPAQUE, &Area, NULL, 0, NULL);
            }
            else
            {
                SetDCPenColor(Global.Memorydevice, Internal::toColor(Command->color));
                SetDCBrushColor(Global.Memorydevice, Internal::toColor(Command->color));
                RoundRect(Global.Memorydevice, Command->x, Command->y, Command->x + Command->w,
                    Command->y + Command->h, Command->rounding, Command->rounding);
            }
        }
        template </*NK_COMMAND_CIRCLE_FILLED*/> void RenderCMD(const nk_command_circle_filled *Command)
        {
            assert(Global.Memorydevice); assert(Command);
            SetDCPenColor(Global.Memorydevice, toColor(Command->color));
            SetDCBrushColor(Global.Memorydevice, toColor(Command->color));
            Ellipse(Global.Memorydevice, Command->x, Command->y, Command->x + Command->w, Command->y + Command->h);
        }
        template </*NK_COMMAND_POLYGON_FILLED*/> void RenderCMD(const nk_command_polygon_filled *Command)
        {
            assert(Global.Memorydevice); assert(Command); assert(Command->point_count);

            const auto Points = (POINT *)alloca(Command->point_count * sizeof(POINT));
            for (int i = 0; i < Command->point_count; ++i)
            {
                Points[i] = { Command->points[i].x, Command->points[i].y };
            }

            SetDCBrushColor(Global.Memorydevice, toColor(Command->color));
            SetDCPenColor(Global.Memorydevice, toColor(Command->color));
            Polygon(Global.Memorydevice, Points, Command->point_count);
        }
        template </*NK_COMMAND_TRIANGLE_FILLED*/> void RenderCMD(const nk_command_triangle_filled *Command)
        {
            assert(Global.Memorydevice); assert(Command);

            POINT Points[3] = { {Command->a.x, Command->a.y}, {Command->b.x, Command->b.y}, {Command->c.x, Command->c.y} };
            SetDCPenColor(Global.Memorydevice, toColor(Command->color));
            SetDCBrushColor(Global.Memorydevice, toColor(Command->color));
            Polygon(Global.Memorydevice, Points, 3);
        }
        template </*NK_COMMAND_RECT_MULTI_COLOR*/> void RenderCMD(const nk_command_rect_multi_color *Command)
        {
            assert(Global.Memorydevice); assert(Command);
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
                GradientFill(Global.Memorydevice, Vertices, 2, &Mesh, 1, GRADIENT_FILL_RECT_H);
            }

            // Vertical gradient.
            else if (0 == std::memcmp(&Command->left, &Command->right, sizeof(nk_color)))
            {
                addColor(Vertices[0], Command->top);
                addColor(Vertices[1], Command->bottom);
                GradientFill(Global.Memorydevice, Vertices, 2, &Mesh, 1, GRADIENT_FILL_RECT_V);
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
        template <> void RenderCMD(const nk_command_arc_filled *Command)
        {
        assert(Global.Memorydevice); assert(Command);
        }
        */
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

                if (Global.Size.x != Width || Global.Size.y != Height)
                {
                    auto Bitmap = CreateCompatibleBitmap(Global.Windowdevice, Width, Height);
                    auto Old = SelectObject(Global.Memorydevice, Bitmap);
                    Global.Size.y = Height;
                    Global.Size.x = Width;
                    DeleteObject(Old);
                }

                return false;
            }
            //case WM_PAINT:
            //{
            //    PAINTSTRUCT Updateinformation{};

            //    const auto Devicecontext = BeginPaint((HWND)Windowhandle, &Updateinformation);
            //    BitBlt(Devicecontext,
            //        Updateinformation.rcPaint.right, Updateinformation.rcPaint.top,
            //        Updateinformation.rcPaint.left - Updateinformation.rcPaint.right,
            //        Updateinformation.rcPaint.bottom - Updateinformation.rcPaint.top,
            //        Global.Memorydevice, 0, 0, SRCCOPY);
            //    EndPaint((HWND)Windowhandle, &Updateinformation);

            //    return true;
            //}

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
                        nk_input_key(&Global.Context, NK_KEY_SHIFT, Down);
                        return true;

                    case VK_DELETE:
                        nk_input_key(&Global.Context, NK_KEY_DEL, Down);
                        return true;

                    case VK_RETURN:
                        nk_input_key(&Global.Context, NK_KEY_ENTER, Down);
                        return true;

                    case VK_TAB:
                        nk_input_key(&Global.Context, NK_KEY_TAB, Down);
                        return true;

                    case VK_LEFT:
                        if (!Ctrl) nk_input_key(&Global.Context, NK_KEY_LEFT, Down);
                        else nk_input_key(&Global.Context, NK_KEY_TEXT_WORD_LEFT, Down);
                        return true;

                    case VK_RIGHT:
                        if (!Ctrl) nk_input_key(&Global.Context, NK_KEY_RIGHT, Down);
                        else nk_input_key(&Global.Context, NK_KEY_TEXT_WORD_RIGHT, Down);
                        return true;

                    case VK_BACK:
                        nk_input_key(&Global.Context, NK_KEY_BACKSPACE, Down);
                        return true;

                    case VK_HOME:
                        nk_input_key(&Global.Context, NK_KEY_TEXT_START, Down);
                        nk_input_key(&Global.Context, NK_KEY_SCROLL_START, Down);
                        return true;

                    case VK_END:
                        nk_input_key(&Global.Context, NK_KEY_TEXT_END, Down);
                        nk_input_key(&Global.Context, NK_KEY_SCROLL_END, Down);
                        return true;

                    case VK_NEXT:
                        nk_input_key(&Global.Context, NK_KEY_SCROLL_DOWN, Down);
                        return true;

                    case VK_PRIOR:
                        nk_input_key(&Global.Context, NK_KEY_SCROLL_UP, Down);
                        return true;

                    case 'C':
                        if (Ctrl)
                        {
                            nk_input_key(&Global.Context, NK_KEY_COPY, Down);
                            return true;
                        }
                        break;

                    case 'V':
                        if (Ctrl)
                        {
                            nk_input_key(&Global.Context, NK_KEY_PASTE, Down);
                            return true;
                        }
                        break;

                    case 'X':
                        if (Ctrl)
                        {
                            nk_input_key(&Global.Context, NK_KEY_CUT, Down);
                            return true;
                        }
                        break;

                    case 'Z':
                        if (Ctrl)
                        {
                            if (Shift) nk_input_key(&Global.Context, NK_KEY_TEXT_REDO, Down);
                            else nk_input_key(&Global.Context, NK_KEY_TEXT_UNDO, Down);
                            return true;
                        }
                        break;

                    case 'R':
                        if (Ctrl)
                        {
                            nk_input_key(&Global.Context, NK_KEY_TEXT_REDO, Down);
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
                    nk_input_unicode(&Global.Context, (nk_rune)wParam);
                    return true;
                }
                break;
            }

            // Mouse input.
            case WM_LBUTTONDOWN:
            {
                nk_input_button(&Global.Context, NK_BUTTON_LEFT, (short)LOWORD(lParam), (short)HIWORD(lParam), 1);
                SetCapture(Windowhandle);
                return true;
            }
            case WM_LBUTTONUP:
            {
                nk_input_button(&Global.Context, NK_BUTTON_DOUBLE, (short)LOWORD(lParam), (short)HIWORD(lParam), 0);
                nk_input_button(&Global.Context, NK_BUTTON_LEFT, (short)LOWORD(lParam), (short)HIWORD(lParam), 0);
                ReleaseCapture();
                return true;
            }
            case WM_RBUTTONDOWN:
            {
                nk_input_button(&Global.Context, NK_BUTTON_RIGHT, (short)LOWORD(lParam), (short)HIWORD(lParam), 1);
                SetCapture(Windowhandle);
                return true;
            }
            case WM_RBUTTONUP:
            {
                nk_input_button(&Global.Context, NK_BUTTON_RIGHT, (short)LOWORD(lParam), (short)HIWORD(lParam), 0);
                ReleaseCapture();
                return true;
            }
            case WM_MBUTTONDOWN:
            {
                nk_input_button(&Global.Context, NK_BUTTON_MIDDLE, (short)LOWORD(lParam), (short)HIWORD(lParam), 1);
                SetCapture(Windowhandle);
                return true;
            }
            case WM_MBUTTONUP:
            {
                nk_input_button(&Global.Context, NK_BUTTON_MIDDLE, (short)LOWORD(lParam), (short)HIWORD(lParam), 0);
                ReleaseCapture();
                return true;
            }
            case WM_MOUSEWHEEL:
            {
                nk_input_scroll(&Global.Context, nk_vec2(0, (float)(short)HIWORD(wParam) / WHEEL_DELTA));
                return true;
            }
            case WM_MOUSEMOVE:
            {
                nk_input_motion(&Global.Context, (short)LOWORD(lParam), (short)HIWORD(lParam));
                return true;
            }
            case WM_LBUTTONDBLCLK:
            {
                nk_input_button(&Global.Context, NK_BUTTON_DOUBLE, (short)LOWORD(lParam), (short)HIWORD(lParam), 1);
                return true;
            }
        }

        return false;
    }

    // Create a centred window chroma-keyed on 0xFFFFFF.
    void *Createwindow()
    {
        // Register the window.
        WNDCLASSEXA Windowclass{};
        Windowclass.cbSize = sizeof(WNDCLASSEXA);
        Windowclass.lpszClassName = "Ayria_overlay";
        Windowclass.hInstance = GetModuleHandleA(NULL);
        Windowclass.hCursor = LoadCursor(NULL, IDC_ARROW);
        Windowclass.hbrBackground = CreateSolidBrush(0x00FFFFFF);
        Windowclass.style = CS_SAVEBITS | CS_BYTEALIGNWINDOW | CS_BYTEALIGNCLIENT |CS_OWNDC;
        Windowclass.lpfnWndProc = [](HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT
        {
            if (onEvent(wnd, msg, wparam, lparam)) return 0;
            return DefWindowProcW(wnd, msg, wparam, lparam);
        };
        if (NULL == RegisterClassExA(&Windowclass)) return nullptr;

        if (const auto Windowhandle = CreateWindowExA(WS_EX_LAYERED, Windowclass.lpszClassName,
            NULL, WS_POPUP, NULL, NULL, NULL, NULL, NULL, NULL, Windowclass.hInstance, NULL))
        {
            // Use a pixel-value of [0xFF, 0xFF, 0xFF] to mean transparent rather than Alpha.
            // Because using Alpha is slow and we should not use pure white anyway.
            SetLayeredWindowAttributes(Windowhandle, 0x00FFFFFF, 0, LWA_COLORKEY);
            return Windowhandle;
        }

        return nullptr;
    }

    std::unordered_set<std::function<void(struct nk_context *)>, decltype(FNV::Hash), decltype(FNV::Equal)> *Callbacks;
    void Registerwindow(std::function<void (struct nk_context *)> Callback)
    {
        if (!Callbacks) Callbacks = new std::remove_pointer_t<decltype(Callbacks)>;
        Callbacks->insert(Callback);
    }

    void onStartup()
    {
        // Windows tracks message-queues by thread ID, so we need to create the window
        // from this new thread to prevent issues. Took like 8 hours of hackery to find that..
        Global.Windowhandle = Createwindow();
        Global.Windowdevice = GetDC((HWND)Global.Windowhandle);
        Global.Drawarea = {};
        Global.isDirty = true;
        Global.Size = {};

        // Arial should be available on every system.
        static auto Systemfont = Fonts::Createfont("Arial", 16);
        nk_init_default(&Global.Context, &Systemfont);

        // Make the window-handle available to other systems.
        Global.Context.userdata = { Global.Windowhandle };

        const auto Bitmap = CreateCompatibleBitmap(Global.Windowdevice, 0, 0);
        Global.Memorydevice = CreateCompatibleDC(Global.Windowdevice);
        SelectObject(Global.Memorydevice, Bitmap);

        // TODO(tcn): More init!
    }
    void onFrame()
    {
        // Track the frame-time, should be less than 33ms.
        static auto Lastframe{ std::chrono::high_resolution_clock::now() };
        const auto Thisframe{ std::chrono::high_resolution_clock::now() };
        const auto Deltatime = std::chrono::duration<float>(Thisframe - Lastframe).count();

        // Update the context for smooth scrolling.
        Global.Context.delta_time_seconds = Deltatime;
        Lastframe = Thisframe;

        // Process window-messages.
        nk_input_begin(&Global.Context);
        {
            MSG Message;
            while (PeekMessageW(&Message, (HWND)Global.Windowhandle, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&Message);
                DispatchMessageW(&Message);
                Global.isDirty = true;
            }
        }
        nk_input_end(&Global.Context);

        // Only redraw when dirty.
        if (Global.isDirty)
        {
            Global.isDirty = false;
            Global.Drawarea = {};

            // Set the default background to white (chroma-keyed to transparent).
            Global.Context.style.window.fixed_background = nk_style_item_color(Clearcolor);
            for (const auto &Item : *Callbacks) Item(&Global.Context);

            // If the draw-area changed between calls, update the windowpos for next frame.
            if (0 != (Global.Drawarea.x + Global.Drawarea.y + Global.Drawarea.z + Global.Drawarea.w))
            {
                SetWindowPos((HWND)Global.Windowhandle, NULL, Global.Drawarea.x, Global.Drawarea.y,
                    Global.Drawarea.z - Global.Drawarea.x, Global.Drawarea.w - Global.Drawarea.y,
                    SWP_NOREDRAW | SWP_DEFERERASE | SWP_NOSENDCHANGING);

                Global.isDirty = true;
            }

            // Process the rendering calls.
            {
                // Ensure that the context is clean.
                SelectObject(Global.Memorydevice, GetStockObject(DC_PEN));
                SelectObject(Global.Memorydevice, GetStockObject(DC_BRUSH));

                // Clear the device.
                {
                    // TODO(tcn): Benchmark the different ways to clear.
                    const RECT Screen = { 0, 0, Global.Size.x, Global.Size.y };
                    SetBkColor(Global.Memorydevice, Internal::toColor(Clearcolor));
                    ExtTextOutW(Global.Memorydevice, 0, 0, ETO_OPAQUE, &Screen, NULL, 0, NULL);
                }

                // Iterate over the render commands.
                auto Command = nk__begin(&Global.Context);
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
                        Case(NK_COMMAND_ARC_FILLED, nk_command_arc_filled);
                        Case(NK_COMMAND_POLYLINE, nk_command_polyline);
                        Case(NK_COMMAND_TRIANGLE, nk_command_triangle);
                        Case(NK_COMMAND_POLYGON, nk_command_polygon);
                        Case(NK_COMMAND_SCISSOR, nk_command_scissor);
                        Case(NK_COMMAND_CIRCLE, nk_command_circle);
                        // Case(NK_COMMAND_CUSTOM, nk_command_custom);
                        Case(NK_COMMAND_CURVE, nk_command_curve);
                        Case(NK_COMMAND_IMAGE, nk_command_image);
                        Case(NK_COMMAND_TEXT, nk_command_text);
                        Case(NK_COMMAND_RECT, nk_command_rect);
                        Case(NK_COMMAND_LINE, nk_command_line);
                        Case(NK_COMMAND_ARC, nk_command_arc);
                        case NK_COMMAND_NOP:
                        default: break;
                            #undef Case
                    }

                    // TODO(tcn): Add a custom allocator to reduce the impact of this linked list.
                    Command = nk__next(&Global.Context, Command);
                }

                // Copy to the screen buffer.
                BitBlt(Global.Windowdevice, 0, 0, Global.Size.x, Global.Size.y, Global.Memorydevice, 0, 0, SRCCOPY);

                // Cleanup.
                nk_clear(&Global.Context);
            }

            // Ensure that the window is marked as visible.
            if (0 != (Global.Drawarea.x + Global.Drawarea.y + Global.Drawarea.z + Global.Drawarea.w))
            {
                ShowWindow((HWND)Global.Windowhandle, SW_SHOWNORMAL);

                // TODO(tcn): Refactor this.
                EnumWindows([](HWND Handle, LPARAM) -> BOOL
                {
                    DWORD ProcessID;
                    const auto ThreadID = GetWindowThreadProcessId(Handle, &ProcessID);

                    if (ProcessID == GetCurrentProcessId() && Handle == GetForegroundWindow())
                    {
                        SetForegroundWindow((HWND)Global.Windowhandle);
                        SetFocus((HWND)Global.Windowhandle);
                        return FALSE;
                    }

                    return TRUE;
                }, NULL);
            }
        }
    }

    void spoil() { Global.isDirty = true; }
    void include(int x0, int y0, int x1, int y1)
    {
        if(0 == (Global.Drawarea.x + Global.Drawarea.y + Global.Drawarea.z + Global.Drawarea.w))
        {
            Global.Drawarea.x = x0; Global.Drawarea.y = y0;
            Global.Drawarea.z = x1; Global.Drawarea.w = y1;
            return;
        }

        if(x0 < Global.Drawarea.x || y0 < Global.Drawarea.y || x1 > (Global.Drawarea.z) || y1 > (Global.Drawarea.w))
        {
            Global.Drawarea.x = Branchless::min(Global.Drawarea.x, x0);
            Global.Drawarea.y = Branchless::min(Global.Drawarea.y, y0);
            Global.Drawarea.z = Branchless::max(Global.Drawarea.z, x1);
            Global.Drawarea.w = Branchless::max(Global.Drawarea.w, y1);
        }
    }
}

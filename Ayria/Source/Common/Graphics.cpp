/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-05-07
    License: MIT

    Quick benchmark for painting a rect (100,000 iterations)
    ExtTextOutW: 765 us
    FillRect: 826 us
    FillRgn: 989 us
    Polygon: 1022 us
    Rectangle: 1028 us
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
    struct Surface_t
    {
        // Returns if the state is spoiled.
        std::function<bool(struct nk_context *)> onRender;
        vec2i Position, Size;
        nk_context Context;
        HWND Nativehandle;
        HDC Windowdevice;
        HDC Memorydevice;

        struct
        {
            uint8_t
                isDirty : 1,
                isVisible : 1,
                needsRefresh : 1;
        } Flags;
    };

    // Forward declaration for the renderer.
    namespace Internal { template <typename CMD> void RenderCMD(const Surface_t *This, const CMD *Command); }
    constexpr COLORREF toColor(const struct nk_color &Color)
    {
        return Color.r | (Color.g << 8) | (Color.b << 16);
    }

    std::unordered_map<std::string, Surface_t> Surfaces;
    constexpr nk_color Clearcolor = { 0xFF, 0xFF, 0xFF, 0xFF };
    const HBRUSH Clearbrush = CreateSolidBrush(toColor(Clearcolor));

    // Internal rendering defs.
    namespace Internal
    {
        template </*NK_COMMAND_ARC*/> void RenderCMD(const Surface_t *This, const nk_command_arc *Command)
        {
            assert(This->Memorydevice); assert(Command);
            HPEN Pen{};

            // Create a custom pen if needed.
            if (Command->line_thickness != 1)
            {
                Pen = CreatePen(PS_SOLID, Command->line_thickness, toColor(Command->color));
                SelectObject(This->Memorydevice, Pen);
            }
            else SetDCPenColor(This->Memorydevice, toColor(Command->color));

            const float X0 = Command->cx + Command->r * std::cosf((Command->a[0] + Command->a[1]) * NK_PI / 180.0f);
            const float Y0 = Command->cy + Command->r * std::sinf((Command->a[0] + Command->a[1]) * NK_PI / 180.0f);
            const float X1 = Command->cx + Command->r * std::cosf(Command->a[0] * NK_PI / 180.0f);
            const float Y1 = Command->cy + Command->r * std::sinf(Command->a[0] * NK_PI / 180.0f);

            SetArcDirection(This->Memorydevice, AD_COUNTERCLOCKWISE);
            Arc(This->Memorydevice, Command->cx - Command->r, Command->cy - Command->r,
                Command->cx + Command->r, Command->cy + Command->r,
                (int)X0, (int)Y0, (int)X1, (int)Y1);

            // Restore.
            if (Pen)
            {
                SelectObject(This->Memorydevice, GetStockObject(DC_PEN));
                DeleteObject(Pen);
            }
        }
        template </*NK_COMMAND_TEXT*/> void RenderCMD(const Surface_t *This, const nk_command_text *Command)
        {
            assert(This->Memorydevice); assert(Command);
            assert(Command->string); assert(Command->font);

            const auto Text = toWide(Command->string, Command->length);
            SetTextColor(This->Memorydevice, toColor(Command->foreground));
            SetBkColor(This->Memorydevice, toColor(Command->background));

            SelectObject(This->Memorydevice, ((Font_t *)Command->font->userdata.ptr)->Fonthandle);
            ExtTextOutW(This->Memorydevice, Command->x, Command->y, ETO_OPAQUE, NULL, Text.c_str(), (UINT)Text.size(), NULL);
        }
        template </*NK_COMMAND_RECT*/> void RenderCMD(const Surface_t *This, const nk_command_rect *Command)
        {
            assert(This->Memorydevice); assert(Command);
            HPEN Pen{};

            // Create a custom pen if needed.
            if (Command->line_thickness != 1)
            {
                Pen = CreatePen(PS_SOLID, Command->line_thickness, toColor(Command->color));
                SelectObject(This->Memorydevice, Pen);
            }
            else SetDCPenColor(This->Memorydevice, toColor(Command->color));

            // No fill.
            const auto Brush = SelectObject(This->Memorydevice, GetStockObject(NULL_BRUSH));
            if (Command->rounding == 0)
                Rectangle(This->Memorydevice, Command->x, Command->y, Command->x + Command->w, Command->y + Command->h);
            else
                RoundRect(This->Memorydevice, Command->x, Command->y, Command->x + Command->w, Command->y + Command->h, Command->rounding, Command->rounding);

            // Restore.
            SelectObject(This->Memorydevice, Brush);
            if (Pen)
            {
                SelectObject(This->Memorydevice, GetStockObject(DC_PEN));
                DeleteObject(Pen);
            }
        }
        template </*NK_COMMAND_LINE*/> void RenderCMD(const Surface_t *This, const nk_command_line *Command)
        {
            assert(This->Memorydevice); assert(Command);
            HPEN Pen{};

            // Create a custom pen if needed.
            if (Command->line_thickness != 1)
            {
                Pen = CreatePen(PS_SOLID, Command->line_thickness, toColor(Command->color));
                SelectObject(This->Memorydevice, Pen);
            }
            else SetDCPenColor(This->Memorydevice, toColor(Command->color));

            // Draw the line.
            MoveToEx(This->Memorydevice, Command->begin.x, Command->begin.y, NULL);
            LineTo(This->Memorydevice, Command->end.x, Command->end.y);

            // Restore.
            if (Pen)
            {
                SelectObject(This->Memorydevice, GetStockObject(DC_PEN));
                DeleteObject(Pen);
            }
        }
        template </*NK_COMMAND_CURVE*/> void RenderCMD(const Surface_t *This, const nk_command_curve *Command)
        {
            assert(This->Memorydevice); assert(Command);
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
                SelectObject(This->Memorydevice, Pen);
            }
            else SetDCPenColor(This->Memorydevice, toColor(Command->color));

            SetDCBrushColor(This->Memorydevice, OPAQUE);
            PolyBezier(This->Memorydevice, Points, 4);

            // Restore.
            if (Pen)
            {
                SelectObject(This->Memorydevice, GetStockObject(DC_PEN));
                DeleteObject(Pen);
            }
        }
        template </*NK_COMMAND_IMAGE*/> void RenderCMD(const Surface_t *This, const nk_command_image *Command)
        {
            assert(This->Memorydevice); assert(Command);

            BITMAP Bitmap{};
            const auto Device = CreateCompatibleDC(This->Memorydevice);
            GetObjectA(Command->img.handle.ptr, sizeof(BITMAP), &Bitmap);

            SelectObject(This->Memorydevice, Command->img.handle.ptr);
            StretchBlt(This->Memorydevice, Command->x, Command->y, Command->w, Command->h,
                Device, 0, 0, Bitmap.bmWidth, Bitmap.bmHeight, SRCCOPY);
            DeleteDC(Device);
        }
        template </*NK_COMMAND_CIRCLE*/> void RenderCMD(const Surface_t *This, const nk_command_circle *Command)
        {
            assert(This->Memorydevice); assert(Command);
            HPEN Pen{};

            // Create a custom pen if needed.
            if (Command->line_thickness != 1)
            {
                Pen = CreatePen(PS_SOLID, Command->line_thickness, toColor(Command->color));
                SelectObject(This->Memorydevice, Pen);
            }
            else SetDCPenColor(This->Memorydevice, toColor(Command->color));

            SetDCBrushColor(This->Memorydevice, OPAQUE);
            Ellipse(This->Memorydevice, Command->x, Command->y, Command->x + Command->w, Command->y + Command->h);

            // Restore.
            if (Pen)
            {
                SelectObject(This->Memorydevice, GetStockObject(DC_PEN));
                DeleteObject(Pen);
            }
        }
        template </*NK_COMMAND_SCISSOR*/> void RenderCMD(const Surface_t *This, const nk_command_scissor *Command)
        {
            assert(This->Memorydevice); assert(Command);

            SelectClipRgn(This->Memorydevice, NULL);
            IntersectClipRect(This->Memorydevice, Command->x, Command->y,
                Command->x + Command->w + 1, Command->y + Command->h + 1);
        }
        template </*NK_COMMAND_POLYGON*/> void RenderCMD(const Surface_t *This, const nk_command_polygon *Command)
        {
            assert(This->Memorydevice); assert(Command); assert(Command->point_count);

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
                SelectObject(This->Memorydevice, Pen);
            }
            else SetDCPenColor(This->Memorydevice, toColor(Command->color));

            Polyline(This->Memorydevice, Points, Command->point_count);

            // Restore.
            if (Pen)
            {
                SelectObject(This->Memorydevice, GetStockObject(DC_PEN));
                DeleteObject(Pen);
            }
        }
        template </*NK_COMMAND_POLYLINE*/> void RenderCMD(const Surface_t *This, const nk_command_polyline *Command)
        {
            assert(This->Memorydevice); assert(Command); assert(Command->point_count);

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
                SelectObject(This->Memorydevice, Pen);
            }
            else SetDCPenColor(This->Memorydevice, toColor(Command->color));

            Polyline(This->Memorydevice, Points, Command->point_count);

            // Restore.
            if (Pen)
            {
                SelectObject(This->Memorydevice, GetStockObject(DC_PEN));
                DeleteObject(Pen);
            }
        }
        template </*NK_COMMAND_TRIANGLE*/> void RenderCMD(const Surface_t *This, const nk_command_triangle *Command)
        {
            assert(This->Memorydevice); assert(Command);
            POINT Points[4] = { {Command->a.x, Command->a.y}, {Command->b.x, Command->b.y},
                {Command->c.x, Command->c.y}, {Command->a.x, Command->a.y} };
            HPEN Pen{};

            // Create a custom pen if needed.
            if (Command->line_thickness != 1)
            {
                Pen = CreatePen(PS_SOLID, Command->line_thickness, toColor(Command->color));
                SelectObject(This->Memorydevice, Pen);
            }
            else SetDCPenColor(This->Memorydevice, toColor(Command->color));

            Polyline(This->Memorydevice, Points, 4);

            // Restore.
            if (Pen)
            {
                SelectObject(This->Memorydevice, GetStockObject(DC_PEN));
                DeleteObject(Pen);
            }
        }
        template </*NK_COMMAND_ARC_FILLED*/> void RenderCMD(const Surface_t *This, const nk_command_arc_filled *Command)
        {
            assert(This->Memorydevice); assert(Command);

            SetDCPenColor(This->Memorydevice, toColor(Command->color));
            SetDCBrushColor(This->Memorydevice, toColor(Command->color));

            const float X0 = Command->cx + Command->r * std::cosf((Command->a[0] + Command->a[1]) * NK_PI / 180.0f);
            const float Y0 = Command->cy + Command->r * std::sinf((Command->a[0] + Command->a[1]) * NK_PI / 180.0f);
            const float X1 = Command->cx + Command->r * std::cosf(Command->a[0] * NK_PI / 180.0f);
            const float Y1 = Command->cy + Command->r * std::sinf(Command->a[0] * NK_PI / 180.0f);

            Pie(This->Memorydevice, Command->cx - Command->r, Command->cy - Command->r,
                Command->cx + Command->r, Command->cy + Command->r,
                (int)X0, (int)Y0, (int)X1, (int)Y1);
        }
        template </*NK_COMMAND_RECT_FILLED*/> void RenderCMD(const Surface_t *This, const nk_command_rect_filled *Command)
        {
            assert(This->Memorydevice); assert(Command);

            // Common case.
            if (Command->rounding == 0)
            {
                // NOTE(tcn): Looks odd but this seems to be the fastest way to clear.
                const RECT Area = { Command->x, Command->y, Command->x + Command->w, Command->y + Command->h };
                SetBkColor(This->Memorydevice, toColor(Command->color));
                ExtTextOutW(This->Memorydevice, 0, 0, ETO_OPAQUE, &Area, NULL, 0, NULL);
            }
            else
            {
                SetDCPenColor(This->Memorydevice, toColor(Command->color));
                SetDCBrushColor(This->Memorydevice, toColor(Command->color));
                RoundRect(This->Memorydevice, Command->x, Command->y, Command->x + Command->w,
                    Command->y + Command->h, Command->rounding, Command->rounding);
            }
        }
        template </*NK_COMMAND_CIRCLE_FILLED*/> void RenderCMD(const Surface_t *This, const nk_command_circle_filled *Command)
        {
            assert(This->Memorydevice); assert(Command);
            SetDCPenColor(This->Memorydevice, toColor(Command->color));
            SetDCBrushColor(This->Memorydevice, toColor(Command->color));
            Ellipse(This->Memorydevice, Command->x, Command->y, Command->x + Command->w, Command->y + Command->h);
        }
        template </*NK_COMMAND_POLYGON_FILLED*/> void RenderCMD(const Surface_t *This, const nk_command_polygon_filled *Command)
        {
            assert(This->Memorydevice); assert(Command); assert(Command->point_count);

            const auto Points = (POINT *)alloca(Command->point_count * sizeof(POINT));
            for (int i = 0; i < Command->point_count; ++i)
            {
                Points[i] = { Command->points[i].x, Command->points[i].y };
            }

            SetDCBrushColor(This->Memorydevice, toColor(Command->color));
            SetDCPenColor(This->Memorydevice, toColor(Command->color));
            Polygon(This->Memorydevice, Points, Command->point_count);
        }
        template </*NK_COMMAND_TRIANGLE_FILLED*/> void RenderCMD(const Surface_t *This, const nk_command_triangle_filled *Command)
        {
            assert(This->Memorydevice); assert(Command);

            POINT Points[3] = { {Command->a.x, Command->a.y}, {Command->b.x, Command->b.y}, {Command->c.x, Command->c.y} };
            SetDCPenColor(This->Memorydevice, toColor(Command->color));
            SetDCBrushColor(This->Memorydevice, toColor(Command->color));
            Polygon(This->Memorydevice, Points, 3);
        }
        template </*NK_COMMAND_RECT_MULTI_COLOR*/> void RenderCMD(const Surface_t *This, const nk_command_rect_multi_color *Command)
        {
            assert(This->Memorydevice); assert(Command);
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
                GradientFill(This->Memorydevice, Vertices, 2, &Mesh, 1, GRADIENT_FILL_RECT_H);
            }

            // Vertical gradient.
            else if (0 == std::memcmp(&Command->left, &Command->right, sizeof(nk_color)))
            {
                addColor(Vertices[0], Command->top);
                addColor(Vertices[1], Command->bottom);
                GradientFill(This->Memorydevice, Vertices, 2, &Mesh, 1, GRADIENT_FILL_RECT_V);
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
        template <> void RenderCMD(const Surface_t *This, const nk_command_arc_filled *Command)
        {
            assert(This->Memorydevice); assert(Command);
        }
        */
    }

    // Returns if the event was handled.
    inline bool onEvent(HWND Windowhandle, UINT Message, WPARAM wParam, LPARAM lParam)
    {
        const auto Result = std::find_if(Surfaces.begin(), Surfaces.end(), [&](const auto &Item) { return Item.second.Nativehandle == Windowhandle; });
        if (Result == Surfaces.end()) return false;
        auto &Surface = Result->second;

        switch (Message)
        {
            // Window-control.
            case WM_SIZE:
            {
                const uint16_t Width = LOWORD(lParam);
                const uint16_t Height = HIWORD(lParam);

                if (Surface.Size.x != Width || Surface.Size.y != Height)
                {
                    auto Bitmap = CreateCompatibleBitmap(Surface.Windowdevice, Width, Height);
                    auto Old = SelectObject(Surface.Memorydevice, Bitmap);
                    Surface.Size.y = Height;
                    Surface.Size.x = Width;
                    DeleteObject(Old);
                }

                return false;
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
                        nk_input_key(&Surface.Context, NK_KEY_SHIFT, Down);
                        return true;

                    case VK_DELETE:
                        nk_input_key(&Surface.Context, NK_KEY_DEL, Down);
                        return true;

                    case VK_RETURN:
                        nk_input_key(&Surface.Context, NK_KEY_ENTER, Down);
                        return true;

                    case VK_TAB:
                        nk_input_key(&Surface.Context, NK_KEY_TAB, Down);
                        return true;

                    case VK_LEFT:
                        if (!Ctrl) nk_input_key(&Surface.Context, NK_KEY_LEFT, Down);
                        else nk_input_key(&Surface.Context, NK_KEY_TEXT_WORD_LEFT, Down);
                        return true;

                    case VK_RIGHT:
                        if (!Ctrl) nk_input_key(&Surface.Context, NK_KEY_RIGHT, Down);
                        else nk_input_key(&Surface.Context, NK_KEY_TEXT_WORD_RIGHT, Down);
                        return true;

                    case VK_UP:
                        nk_input_key(&Surface.Context, NK_KEY_UP, Down);
                        return true;

                    case VK_DOWN:
                        nk_input_key(&Surface.Context, NK_KEY_DOWN, Down);
                        return true;

                    case VK_BACK:
                        nk_input_key(&Surface.Context, NK_KEY_BACKSPACE, Down);
                        return true;

                    case VK_HOME:
                        nk_input_key(&Surface.Context, NK_KEY_TEXT_START, Down);
                        nk_input_key(&Surface.Context, NK_KEY_SCROLL_START, Down);
                        return true;

                    case VK_END:
                        nk_input_key(&Surface.Context, NK_KEY_TEXT_END, Down);
                        nk_input_key(&Surface.Context, NK_KEY_SCROLL_END, Down);
                        return true;

                    case VK_NEXT:
                        nk_input_key(&Surface.Context, NK_KEY_SCROLL_DOWN, Down);
                        return true;

                    case VK_PRIOR:
                        nk_input_key(&Surface.Context, NK_KEY_SCROLL_UP, Down);
                        return true;

                    case 'C':
                        if (Ctrl)
                        {
                            nk_input_key(&Surface.Context, NK_KEY_COPY, Down);
                            return true;
                        }
                        break;

                    case 'V':
                        if (Ctrl)
                        {
                            nk_input_key(&Surface.Context, NK_KEY_PASTE, Down);
                            return true;
                        }
                        break;

                    case 'X':
                        if (Ctrl)
                        {
                            nk_input_key(&Surface.Context, NK_KEY_CUT, Down);
                            return true;
                        }
                        break;

                    case 'Z':
                        if (Ctrl)
                        {
                            if (Shift) nk_input_key(&Surface.Context, NK_KEY_TEXT_REDO, Down);
                            else nk_input_key(&Surface.Context, NK_KEY_TEXT_UNDO, Down);
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
                    nk_input_unicode(&Surface.Context, (nk_rune)wParam);
                    return true;
                }
                break;
            }

            // Mouse input.
            case WM_LBUTTONDOWN:
            {
                nk_input_button(&Surface.Context, NK_BUTTON_LEFT, (short)LOWORD(lParam), (short)HIWORD(lParam), 1);
                SetCapture(Windowhandle);
                return true;
            }
            case WM_LBUTTONUP:
            {
                nk_input_button(&Surface.Context, NK_BUTTON_DOUBLE, (short)LOWORD(lParam), (short)HIWORD(lParam), 0);
                nk_input_button(&Surface.Context, NK_BUTTON_LEFT, (short)LOWORD(lParam), (short)HIWORD(lParam), 0);
                ReleaseCapture();
                return true;
            }
            case WM_RBUTTONDOWN:
            {
                nk_input_button(&Surface.Context, NK_BUTTON_RIGHT, (short)LOWORD(lParam), (short)HIWORD(lParam), 1);
                SetCapture(Windowhandle);
                return true;
            }
            case WM_RBUTTONUP:
            {
                nk_input_button(&Surface.Context, NK_BUTTON_RIGHT, (short)LOWORD(lParam), (short)HIWORD(lParam), 0);
                ReleaseCapture();
                return true;
            }
            case WM_MBUTTONDOWN:
            {
                nk_input_button(&Surface.Context, NK_BUTTON_MIDDLE, (short)LOWORD(lParam), (short)HIWORD(lParam), 1);
                SetCapture(Windowhandle);
                return true;
            }
            case WM_MBUTTONUP:
            {
                nk_input_button(&Surface.Context, NK_BUTTON_MIDDLE, (short)LOWORD(lParam), (short)HIWORD(lParam), 0);
                ReleaseCapture();
                return true;
            }
            case WM_MOUSEWHEEL:
            {
                nk_input_scroll(&Surface.Context, nk_vec2(0, (float)(short)HIWORD(wParam) / WHEEL_DELTA));
                return true;
            }
            case WM_MOUSEMOVE:
            {
                nk_input_motion(&Surface.Context, (short)LOWORD(lParam), (short)HIWORD(lParam));
                return true;
            }
            case WM_LBUTTONDBLCLK:
            {
                nk_input_button(&Surface.Context, NK_BUTTON_DOUBLE, (short)LOWORD(lParam), (short)HIWORD(lParam), 1);
                return true;
            }
        }

        return false;
    }
    inline void onTick(float Deltatime)
    {
        for(auto &[Name, Surface] : Surfaces)
        {
            // Update the context for smooth scrolling.
            Surface.Context.delta_time_seconds = Deltatime;

            // Process window-messages.
            nk_input_begin(&Surface.Context);
            {
                MSG Message;
                Surface.Flags.isDirty = false;

                while (PeekMessageW(&Message, Surface.Nativehandle, 0, 0, PM_REMOVE))
                {
                    TranslateMessage(&Message);
                    DispatchMessageW(&Message);
                    Surface.Flags.isDirty = true;
                }
            }
            nk_input_end(&Surface.Context);

            // Only update the context when processed new messages.
            if ((Surface.Flags.isDirty || Surface.Flags.needsRefresh) && Surface.onRender)
            {
                // Set the default background to white (chroma-keyed to transparent).
                Surface.Context.style.window.fixed_background = nk_style_item_color(Clearcolor);
                Surface.onRender(&Surface.Context);
            }
        }
    }
    inline void onPaint()
    {
        for (auto &[Name, Surface] : Surfaces)
        {
            if (Surface.Flags.isDirty || Surface.Flags.needsRefresh)
            {
                if (Surface.Flags.isVisible)
                {
                    // Ensure that the context is clean.
                    SelectObject(Surface.Memorydevice, GetStockObject(DC_PEN));
                    SelectObject(Surface.Memorydevice, GetStockObject(DC_BRUSH));

                    // NOTE(tcn): This seems to be the fastest way to clear.
                    const RECT Screen = { 0, 0, Surface.Size.x, Surface.Size.y };
                    SetBkColor(Surface.Memorydevice, toColor(Clearcolor));
                    ExtTextOutW(Surface.Memorydevice, 0, 0, ETO_OPAQUE, &Screen, NULL, 0, NULL);

                    // Iterate over the render commands.
                    auto Command = nk__begin(&Surface.Context);
                    while (Command)
                    {
                        switch (Command->type)
                        {
                            #define Case(x, y) case x: Internal::RenderCMD(&Surface, (y *)Command); break;
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
                        Command = nk__next(&Surface.Context, Command);
                    }

                    // Copy to the screen buffer.
                    BitBlt(Surface.Windowdevice, 0, 0, Surface.Size.x, Surface.Size.y, Surface.Memorydevice, 0, 0, SRCCOPY);
                }

                // Cleanup.
                nk_clear(&Surface.Context);
                Surface.Flags.needsRefresh = false;
            }

            // Ensure that the window-state matches ours.
            ShowWindowAsync(Surface.Nativehandle, Surface.Flags.isVisible ? SW_SHOWNORMAL : SW_HIDE);

            // Verify that the window is focused.
            if (Surface.Flags.isVisible)
            {
                EnumWindows([](HWND Handle, LPARAM Surface) -> BOOL
                {
                    DWORD ProcessID;
                    const auto ThreadID = GetWindowThreadProcessId(Handle, &ProcessID);

                    if (ProcessID == GetCurrentProcessId() && Handle == GetForegroundWindow())
                    {
                        SetForegroundWindow((HWND)Surface);
                        SetFocus((HWND)Surface);
                        return FALSE;
                    }

                    return TRUE;
                }, (LPARAM)Surface.Nativehandle);
            }
        }
    }

    // Exported functionality.
    bool Createsurface(std::string_view Classname, std::function<bool(struct nk_context *)> onRender)
    {
        // Register the window.
        WNDCLASSEXA Windowclass{};
        Windowclass.hbrBackground = Clearbrush;
        Windowclass.cbSize = sizeof(WNDCLASSEXA);
        Windowclass.lpszClassName = Classname.data();
        Windowclass.hInstance = GetModuleHandleA(NULL);
        Windowclass.style = CS_SAVEBITS | CS_BYTEALIGNWINDOW | CS_BYTEALIGNCLIENT | CS_OWNDC;
        Windowclass.lpfnWndProc = [](HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT
        {
            if (onEvent(wnd, msg, wparam, lparam)) return 0;
            return DefWindowProcW(wnd, msg, wparam, lparam);
        };
        if (NULL == RegisterClassExA(&Windowclass)) return false;

        const auto Windowhandle = CreateWindowExA(WS_EX_LAYERED | WS_EX_TOOLWINDOW,
            Windowclass.lpszClassName, NULL, WS_POPUP, NULL, NULL, NULL, NULL, NULL, NULL, Windowclass.hInstance, NULL);
        if (!Windowhandle) return false;

        // Use a pixel-value to mean transparent rather than Alpha, because using Alpha is slow.
        SetLayeredWindowAttributes(Windowhandle, 0x00FFFFFF, 0, LWA_COLORKEY);

        Surface_t Surface;
        Surface.Size = {};
        Surface.Position = {};
        Surface.onRender = onRender;
        Surface.Flags.isDirty = true;
        Surface.Flags.isVisible = false;
        Surface.Flags.needsRefresh = false;
        Surface.Nativehandle = Windowhandle;
        Surface.Windowdevice = GetDC(Windowhandle);

        // Consolas should be available on every windows system (Vista+).
        static auto Systemfont = Fonts::Createfont("Consolas", 16);
        nk_init_default(&Surface.Context, &Systemfont);
        Surface.Context.userdata = { Windowhandle };
        Surface.Context.clip.paste = [](nk_handle, struct nk_text_edit *Edit)
        {
            if (IsClipboardFormatAvailable(CF_UNICODETEXT) && OpenClipboard(NULL))
            {
                if (const auto Memory = GetClipboardData(CF_UNICODETEXT))
                {
                    if (const auto Size = GlobalSize(Memory) - 1)
                    {
                        if(const auto String = (LPCWSTR)GlobalLock(Memory))
                        {
                            const auto Narrow = toNarrow(String, Size / sizeof(wchar_t));
                            nk_textedit_paste(Edit, Narrow.c_str(), Narrow.size());
                        }
                    }

                    GlobalUnlock(Memory);
                }

                CloseClipboard();
            }
        };

        // Register the surface with windows.
        const auto Bitmap = CreateCompatibleBitmap(Surface.Windowdevice, 0, 0);
        Surface.Memorydevice = CreateCompatibleDC(Surface.Windowdevice);
        DeleteObject(SelectObject(Surface.Memorydevice, Bitmap));

        // Only insert when ready.
        Surfaces[Classname.data()] = Surface;
        return true;
    }
    void setVisibility(std::string_view Classname, bool Visible)
    {
        assert(Surfaces.contains(Classname.data()));
        Surfaces[Classname.data()].Flags.isVisible = Visible;
        Surfaces[Classname.data()].Flags.needsRefresh = true;
    }
    void onFrame()
    {
        // Track the frame-time, should be less than 33ms.
        static auto Lastframe{ std::chrono::high_resolution_clock::now() };
        const auto Thisframe{ std::chrono::high_resolution_clock::now() };
        const auto Deltatime = std::chrono::duration<float>(Thisframe - Lastframe).count();

        Lastframe = Thisframe;
        onTick(Deltatime);
        onPaint();
    }

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
        constexpr size_t Bitsperpixel = 24;
        constexpr size_t Pixelsize = 3;

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
}

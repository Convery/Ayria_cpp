/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-09-18
    License: MIT

    Simple console based on Quake, totally not copy-pasted.
*/

#include <Stdinclude.hpp>
#include <Global.hpp>

namespace Console
{
    namespace Windows
    {
        // Each window needs a unique ID, because reasons.
        constexpr size_t InputID = 1, BufferID = 2;
        HWND Consolehandle, Inputhandle, Bufferhandle;
        uint32_t Lastmessage;
        vec2_t Windowsize;
        WNDPROC oldLine;

        LRESULT __stdcall Inputproc(HWND Handle, UINT Message, WPARAM wParam, LPARAM lParam)
        {
            if (Message == WM_SETFOCUS) SendMessageW(Bufferhandle, EM_SETSEL, (WPARAM)-1, -1);
            if (Message == WM_CHAR && wParam == VK_RETURN)
            {
                wchar_t Input[1024]{};
                GetWindowTextW(Inputhandle, Input, 1024);
                SetWindowTextW(Inputhandle, L"");
                Console::execCommandline(Input);
                return 0;
            }

            return CallWindowProcW(oldLine, Handle, Message, wParam, lParam);
        }
        LRESULT __stdcall Consoleproc(HWND Handle, UINT Message, WPARAM wParam, LPARAM lParam)
        {
            if (Message == WM_TIMER) [[likely]]
            {
                const auto Hash = Hash::FNV1_32(Console::getLoglines(1, L"")[0].first);

                if (Lastmessage != Hash)
                {
                    // If the user have selected text, skip.
                    const auto Pos = SendMessageW(Bufferhandle, EM_GETSEL, NULL, NULL);
                    if (HIWORD(Pos) == LOWORD(Pos))
                    {
                        std::wstring Concatenated;
                        Lastmessage = Hash;

                        for (const auto &[String, Colour] : Console::getLoglines(999, L""))
                        {
                            if (String.empty()) continue;
                            Concatenated += String;
                            Concatenated += L"\r\n";
                        }

                        SetWindowTextW(Bufferhandle, Concatenated.c_str());
                        SendMessageW(Bufferhandle, WM_VSCROLL, SB_BOTTOM, 0);
                    }
                }
            }
            else if (Message == WM_NCLBUTTONDOWN)
            {
                SendMessageW(Bufferhandle, EM_SETSEL, (WPARAM)-1, -1);
                SetFocus(Inputhandle);
            }
            else if (Message == WM_ACTIVATE)
            {
                if (LOWORD(wParam) == WA_INACTIVE) SendMessageW(Bufferhandle, EM_SETSEL, (WPARAM)-1, -1);
                SetFocus(Inputhandle);
            }

            return DefWindowProcW(Handle, Message, wParam, lParam);
        }

        DWORD __stdcall Consolethread(void *)
        {
            MSG Message;
            Createconsole(GetModuleHandleA(NULL));

            while (GetMessageW(&Message, 0, 0, 0))
            {
                TranslateMessage(&Message);
                DispatchMessageW(&Message);
            }

            return 0;
        }
        void Createconsole(HINSTANCE hInstance)
        {
            constexpr DWORD Style = WS_POPUPWINDOW | WS_CAPTION | WS_MINIMIZEBOX;

            WNDCLASSW Windowclass{};
            Windowclass.hInstance = hInstance;
            Windowclass.lpfnWndProc = Consoleproc;
            Windowclass.lpszClassName = L"Windows_console";
            Windowclass.hbrBackground = (HBRUSH)COLOR_WINDOW;
            Windowclass.hIcon = LoadIconW(hInstance, (LPCWSTR)1);
            if (!RegisterClassW(&Windowclass)) assert(false); // WTF?

            const auto Device = GetDC(GetDesktopWindow());
            auto Height = GetDeviceCaps(Device, VERTRES);
            auto Width = GetDeviceCaps(Device, HORZRES);
            ReleaseDC(GetDesktopWindow(), Device);

            RECT Windowrect{ 0, 0, 820, 450 };
            AdjustWindowRect(&Windowrect, Style, FALSE);

            const vec2_t Position{ (Width - 150) / 2, (Height - 450) / 2 };
            Windowsize = { Windowrect.right - Windowrect.left + 1,  Windowrect.bottom - Windowrect.top + 1 };

            Consolehandle = CreateWindowExW(NULL, Windowclass.lpszClassName, L"Console", Style,
                Position.x, Position.y, Windowsize.x, Windowsize.y, NULL, NULL, hInstance, NULL);
            assert(Consolehandle); // WTF?
            SetTimer(Consolehandle, Hash::FNV1_32("Tick"), 300, NULL);

            constexpr auto Linestyle = WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL;
            constexpr auto Bufferstyle = WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER | ES_LEFT | ES_MULTILINE | ES_READONLY | ES_NOHIDESEL;
            Bufferhandle = CreateWindowExW(NULL, L"edit", NULL, Bufferstyle, 6, 5, 806, 410, Consolehandle, (HMENU)BufferID, hInstance, NULL);
            Inputhandle = CreateWindowExW(NULL, L"edit", NULL, Linestyle, 6, 420, 808, 20, Consolehandle, (HMENU)InputID, hInstance, NULL);

            SendMessageW(Bufferhandle, WM_SETFONT, (WPARAM)getDefaultfont(), NULL);
            SendMessageW(Inputhandle, WM_SETFONT, (WPARAM)getDefaultfont(), NULL);

            oldLine = (WNDPROC)SetWindowLongPtrW(Inputhandle, GWLP_WNDPROC, (LONG_PTR)Inputproc);
            SetFocus(Inputhandle);
        }
        void Showconsole(bool Hide)
        {
            if (!Consolehandle) CreateThread(NULL, NULL, Consolethread, NULL, STACK_SIZE_PARAM_IS_A_RESERVATION, NULL);
            while (!Consolehandle) std::this_thread::yield();

            ShowCursor(!Hide);
            ShowWindow(Consolehandle, Hide ? SW_HIDE : SW_SHOWNORMAL);
        }
    }
}

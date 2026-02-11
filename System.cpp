#include "System.h"
#include "Utils/Debug/Logger.h"
#include <windows.h>
#include <ShlObj.h>
#undef ERROR

namespace System {
    void ChangeWallpaper(const wchar_t* filepath) {
        wchar_t absolutePath[MAX_PATH];
        // filepath is relative, make it absolute
        size_t convertedChars = 0;
        if (auto err = mbstowcs_s(&convertedChars, absolutePath, MAX_PATH, __FILE__, sizeof(__FILE__) - sizeof("System.cpp"))) {
            Quasi::Debug::QError$("Error Converting filepath, code = {}", err);
        }
        --convertedChars; // one less to overwrite null terminator
        wcscpy(absolutePath + convertedChars, L"res/");
        convertedChars += 4;
        if (auto err = wcscpy_s(absolutePath + convertedChars, sizeof(absolutePath) - convertedChars, filepath)) {
            Quasi::Debug::QError$("Error Converting filepath, code = {}", err);
        }

        // Call SystemParametersInfoW to set the desktop wallpaper
        // Parameters:
        // SPI_SETDESKWALLPAPER: Sets the desktop wallpaper
        // 0: Not used for this action
        // (void*)wallpaperPath: Pointer to the path string
        // SPIF_UPDATEINIFILE | SPIF_SENDCHANGE: Updates the user profile and broadcasts a message
        //                                         to all windows to inform them of the change.
        BOOL result = SystemParametersInfoW(
            SPI_SETDESKWALLPAPER,
            0,
            absolutePath,
            SPIF_UPDATEINIFILE | SPIF_SENDCHANGE
        );
        if (!result) {
            Quasi::Debug::QError$("Failed to set wallpaper!");
        }
    }

    void HideIcons() {
        SHELLSTATE ss;
        ss.fHideIcons = true;
        SHGetSetSettings(&ss, SSF_HIDEICONS, true);
    }

    void HideAllWindows() {
        HWND lHwnd = FindWindow("Shell_TrayWnd", NULL);
        SendMessage(lHwnd, WM_COMMAND, 419, 0);
    }

    void Shutdown() {
        // kinda unsafe. if you dont want this, turn safe mode on in cmakelists.
        std::system("shutdown /s /t 0");
    }

    void HideTaskbar() {
        static HWND hShellWnd = FindWindow("Shell_TrayWnd", NULL);
        ShowWindow(hShellWnd, SW_HIDE);
    }

    void ShowTaskbar() {
        static HWND hShellWnd = FindWindow("Shell_TrayWnd", NULL);
        ShowWindow(hShellWnd, SW_SHOW);
    }

    Image CaptureScreen() {
        static constexpr int w = 1920, h = 1080;
        HDC     hScreen = GetDC(NULL);
        HDC     hDC     = CreateCompatibleDC(hScreen);
        HBITMAP hBitmap = CreateCompatibleBitmap(hScreen, w, h);
        HGDIOBJ old_obj = SelectObject(hDC, hBitmap);
        if (!BitBlt(hDC, 0, 0, w, h, hScreen, 0, 0, SRCCOPY))
            Quasi::Debug::QError$("Failed to capture screen!");

        BITMAP Bmp = {};
        BITMAPINFO Info = {};

        GetObject(hBitmap, sizeof(Bmp), &Bmp);

        int width, height;
        Info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        Info.bmiHeader.biWidth = width = Bmp.bmWidth;
        Info.bmiHeader.biHeight = height = Bmp.bmHeight;
        Info.bmiHeader.biPlanes = 1;
        Info.bmiHeader.biBitCount = Bmp.bmBitsPixel;
        Info.bmiHeader.biCompression = BI_RGB;
        Info.bmiHeader.biSizeImage = ((width * Bmp.bmBitsPixel + 31) / 32) * 4 * height;

        using namespace Quasi::Graphics;
        Image screenCapture = Image::New(width, height);
        GetDIBits(hDC, hBitmap, 0, height, screenCapture.Data(), &Info, DIB_RGB_COLORS);

        // clean up
        SelectObject(hDC, old_obj);
        DeleteDC(hDC);
        ReleaseDC(NULL, hScreen);
        DeleteObject(hBitmap);

        return screenCapture;
    }
}

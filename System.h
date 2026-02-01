#pragma once
#include "Image.h"

namespace System {
    void ChangeWallpaper(const wchar_t* filepath);
    void HideIcons();
    void HideAllWindows();

    void HideTaskbar();
    void ShowTaskbar();

    Quasi::Graphics::Image CaptureScreen();
}

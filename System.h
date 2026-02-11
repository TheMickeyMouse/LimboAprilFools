#pragma once
#include "Image.h"

using namespace Quasi::Graphics;
namespace System {
    void ChangeWallpaper(const wchar_t* filepath);
    void HideIcons();
    void HideAllWindows();
    void Shutdown();
    void HideTaskbar();
    void ShowTaskbar();

    Image CaptureScreen();
}

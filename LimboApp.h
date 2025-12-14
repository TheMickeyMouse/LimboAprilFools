#pragma once
#include "GUI/Canvas.h"
#include "Quasi/src/Graphics/GraphicsDevice.h"

using namespace Quasi;

struct LimboKey {
    Math::fv2 position;
};

class LimboApp {
    static constexpr float WIDTH = 1920, HEIGHT = 1080;

    Graphics::GraphicsDevice gdevice = Graphics::GraphicsDevice::Initialize({ (int)WIDTH, (int)HEIGHT }, { .transparent = true });
    Graphics::Canvas canvas { gdevice };

    LimboKey keys[8] = {
        {{ WIDTH * 0.2f, HEIGHT * 0.65f }},
        {{ WIDTH * 0.4f, HEIGHT * 0.65f }},
        {{ WIDTH * 0.6f, HEIGHT * 0.65f }},
        {{ WIDTH * 0.8f, HEIGHT * 0.65f }},
        {{ WIDTH * 0.2f, HEIGHT * 0.35f }},
        {{ WIDTH * 0.4f, HEIGHT * 0.35f }},
        {{ WIDTH * 0.6f, HEIGHT * 0.35f }},
        {{ WIDTH * 0.8f, HEIGHT * 0.35f }},
    };

    Graphics::Texture2D texKeyMain, texKeyHigh, texKeyShadow, texKeyOutline;
    Graphics::Image colorPalette;
public:
    LimboApp();

    bool Run();
    void DrawKeys();
};
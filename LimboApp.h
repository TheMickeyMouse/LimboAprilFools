#pragma once
#include "GUI/Canvas.h"
#include "Quasi/src/Graphics/GraphicsDevice.h"

using namespace Quasi;

struct LimboKey {
    Math::fv3 position;
};

class LimboApp {
    static constexpr float WIDTH = 1920, HEIGHT = 1080, Z_CENTER = 1.0f;

    Graphics::GraphicsDevice gdevice;
    Graphics::Canvas canvas { gdevice };

    LimboKey keys[8] = {
        {{ WIDTH * 0.2f, HEIGHT * 0.65f, Z_CENTER }},
        {{ WIDTH * 0.4f, HEIGHT * 0.65f, Z_CENTER }},
        {{ WIDTH * 0.6f, HEIGHT * 0.65f, Z_CENTER }},
        {{ WIDTH * 0.8f, HEIGHT * 0.65f, Z_CENTER }},
        {{ WIDTH * 0.2f, HEIGHT * 0.35f, Z_CENTER }},
        {{ WIDTH * 0.4f, HEIGHT * 0.35f, Z_CENTER }},
        {{ WIDTH * 0.6f, HEIGHT * 0.35f, Z_CENTER }},
        {{ WIDTH * 0.8f, HEIGHT * 0.35f, Z_CENTER }},
    };

    Graphics::Texture2D texKeyMain, texKeyHigh, texKeyShadow, texKeyOutline;
    Math::fColor colorPalette[8][3];
public:
    LimboApp();

    bool Run();
    void DrawKey(int index, float scale = 200);
    void DrawKeys();
    static Math::fv2 Project(Math::fv3 position);

    void SetSpinningKeys();
};
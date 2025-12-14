#include "LimboApp.h"

#include "glp.h"

LimboApp::LimboApp() {
    texKeyHigh    = Graphics::Texture2D::LoadPNG("../keyhigh.png");
    texKeyMain    = Graphics::Texture2D::LoadPNG("../keymain.png");
    texKeyShadow  = Graphics::Texture2D::LoadPNG("../keyshadow.png");
    texKeyOutline = Graphics::Texture2D::LoadPNG("../keyoutline.png");
    colorPalette = Graphics::Image::LoadPNG("../colorpalette.png");

    Graphics::Render::UseBlendFunc(Graphics::BlendFactor::ONE, Graphics::BlendFactor::INVERT_SRC_ALPHA);
}

bool LimboApp::Run() {
    gdevice.Begin();
    canvas.BeginFrame();

    canvas.Fill(Math::fColor::White());
    canvas.DrawRect({ { WIDTH * 0.05, HEIGHT * 0.15 }, { WIDTH * 0.95, HEIGHT * 0.85 } });
    // canvas.DrawText("Hello, World!", 60, { 560, 740 }, { .rect = { 800, 400 } });
    DrawKeys();

    // jailbreak safety measure
    const auto& io = gdevice.GetIO();
    if (io.Keyboard.KeyOnPress(IO::Key::F)) {
        return false;
    }

    canvas.EndFrame();
    gdevice.End();
    return true;
}

void LimboApp::DrawKeys() {
    for (int i = 0; i < 8; i++) {
        canvas.DrawTextureW(texKeyMain,    keys[i].position, 200, true, { .tint = (Math::fColor)colorPalette[{ 0, 7 - i }] });
        canvas.DrawTextureW(texKeyHigh,    keys[i].position, 200, true, { .tint = (Math::fColor)colorPalette[{ 1, 7 - i }] });
        canvas.DrawTextureW(texKeyShadow,  keys[i].position, 200, true, { .tint = (Math::fColor)colorPalette[{ 2, 7 - i }] });
        canvas.DrawTextureW(texKeyOutline, keys[i].position, 200);
    }
}

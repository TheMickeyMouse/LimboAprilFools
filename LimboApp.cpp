#include "LimboApp.h"

LimboApp::LimboApp() : gdevice(Graphics::GraphicsDevice::Initialize({ (int)WIDTH, (int)HEIGHT }, { .transparent = true })) {
    // const Graphics::TextureLoadParams params = { .pixelated = true };
    texKeyHigh    = Graphics::Texture2D::LoadPNG("../keyhighfix.png");
    texKeyMain    = Graphics::Texture2D::LoadPNG("../keymainfix.png");
    texKeyShadow  = Graphics::Texture2D::LoadPNG("../keyshadowfix.png");
    texKeyOutline = Graphics::Texture2D::LoadPNG("../keyoutline.png");
    Graphics::Image colorSrc = Graphics::Image::LoadPNG("../colorpalette.png");

    for (int i = 0; i < 8; ++i) {
        for (int tone = 0; tone < 3; ++tone) {
            // image is flipped
            colorPalette[i][tone] = (Math::fColor)colorSrc[{ tone, 7 - i }];
        }
    }

    Graphics::Render::UseBlendFunc(Graphics::BlendFactor::ONE, Graphics::BlendFactor::INVERT_SRC_ALPHA);
}

bool LimboApp::Run() {
    gdevice.Begin();
    canvas.BeginFrame();

    // canvas.Fill(Math::fColor::White());
    // canvas.DrawRect({ { WIDTH * 0.05, HEIGHT * 0.15 }, { WIDTH * 0.95, HEIGHT * 0.85 } });
    // canvas.DrawText("Hello, World!", 60, { 560, 740 }, { .rect = { 800, 400 } });

    SetSpinningKeys();

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

void LimboApp::DrawKey(int index, float scale) {
    const Math::fv2 screenPos = Project(keys[index].position);
    const float size = scale * Z_CENTER / keys[index].position.z;
    canvas.DrawTextureW(texKeyMain,    screenPos, size, true, { .tint = colorPalette[index][0] });
    canvas.DrawTextureW(texKeyHigh,    screenPos, size, true, { .tint = colorPalette[index][1] });
    canvas.DrawTextureW(texKeyShadow,  screenPos, size, true, { .tint = colorPalette[index][2] });
    canvas.DrawTextureW(texKeyOutline, screenPos, size);
}

void LimboApp::DrawKeys() {
    for (int i = 0; i < 8; i++) { // back keys
        if (keys[i].position.z < Z_CENTER) continue;
        DrawKey(i);
    }

    canvas.Stroke(Math::fColor::White());
    canvas.DrawText("Limbo", 216, { WIDTH * 0.3, HEIGHT * 0.7 }, { .rect = { WIDTH * 0.4, HEIGHT * 0.4 } });

    for (int i = 0; i < 8; i++) { // front keys
        if (keys[i].position.z > Z_CENTER) continue;
        DrawKey(i);
    }
}

Math::fv2 LimboApp::Project(Math::fv3 position) {
    const Math::fv2 center = { WIDTH / 2, HEIGHT / 2 };
    return center + (position.As2D() - center) * (Z_CENTER / position.z);
}

void LimboApp::SetSpinningKeys() {
    const float time = gdevice.GetIO().Time.currentTime;
    // const float time = 0;

    using namespace Quasi::Math;
    const Radians AXIS_TILT = 85.0_deg;
    const fv3 mainAxis = fv3::FromSpheric(1, Radians(time * 0.3f), AXIS_TILT);
    const Rotor3D tilt = Rotor3D::RotateTo({ 0, 1, 0 }, mainAxis);

    const Rotor2D turn = 45.0_deg;
    Rotor2D curr = Radians(time * 0.75f);

    const fv3 origin = { WIDTH * 0.5f, HEIGHT * 0.5f, Z_CENTER };
    fv3 localX = tilt * fv3 { WIDTH * 0.3f, 0, 0 };
    fv3 localZ = tilt * fv3 { 0, WIDTH * 0.1f, WIDTH * 0.1f };
    localX.z = 0;
    localZ.z /= WIDTH * 0.33f;
    for (auto& key : keys) {
        key.position = localX * curr.Cos() + localZ * curr.Sin() + origin;
        curr += turn;
    }

    // canvas.DrawLine({ WIDTH * 0.5f, HEIGHT * 0.5f }, Project(mainAxis * fv3 { 300, 300, 10 } + origin));
}

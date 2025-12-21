#include "LimboApp.h"
#include "Utils/Algorithm.h"

// A B C D
// E F G H
#define P_SWAP_ROWS  "EFGHABCD"
#define P_SWAP_COLS  "BADCFEHG"
#define P_SHIFT_CW   "EABCFGHD"
#define P_SHIFT_CCW  "BCDHAEFG"
#define P_CYCLE_CELL "BFGCAEHD"
#define P_EASY_SHIFT "ECBHAGFD"

const Math::fv2 LimboApp::ORIGIN = { WIDTH / 2, HEIGHT / 2 };
const Math::fv2 LimboApp::TARGET_POSITIONS[8] = {
    { WIDTH * 0.2f, HEIGHT * 0.65f },
    { WIDTH * 0.4f, HEIGHT * 0.65f },
    { WIDTH * 0.6f, HEIGHT * 0.65f },
    { WIDTH * 0.8f, HEIGHT * 0.65f },
    { WIDTH * 0.2f, HEIGHT * 0.35f },
    { WIDTH * 0.4f, HEIGHT * 0.35f },
    { WIDTH * 0.6f, HEIGHT * 0.35f },
    { WIDTH * 0.8f, HEIGHT * 0.35f },
};

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

    ResetKeyPos();

#define FACTORY(X) [] () -> Box<Permutation> { return X; }
    {
        permutations = Vecs::New((Box<Permutation>(*[])()) {
            FACTORY(Box<RotatePerm>::Build())
        });
    }

    Graphics::Render::UseBlendFunc(Graphics::BlendFactor::ONE, Graphics::BlendFactor::INVERT_SRC_ALPHA);
}

bool LimboApp::Run() {
    gdevice.Begin();
    canvas.BeginFrame();

    // canvas.Fill(Math::fColor::White());
    // canvas.DrawRect({ { WIDTH * 0.05, HEIGHT * 0.15 }, { WIDTH * 0.95, HEIGHT * 0.85 } });
    // canvas.DrawText("Hello, World!", 60, { 560, 740 }, { .rect = { 800, 400 } });
    const auto& io = gdevice.GetIO();

    // SetSpinningKeys();
    if (currentPerm)
        currentPerm->Anim(*this, io.Time.DeltaTime());
    DrawKeys();

    // jailbreak safety measure
    if (io.Keyboard.KeyOnPress(IO::Key::F)) {
        return false;
    }

    if (io.Keyboard.KeyOnPress(IO::Key::P)) {
        if (currentPerm) currentPerm->Finish(*this);
        currentPerm = permutations[frame]();
        ++frame %= permutations.Length();
    }

    canvas.EndFrame();
    gdevice.End();
    return true;
}

void LimboApp::DrawKey(int index, float scale, int overrideColorIndex) {
    overrideColorIndex = overrideColorIndex == -1 ? index : overrideColorIndex;
    const Math::fv2 screenPos = Project(keys[index].position, keys[index].z);
    const float size = scale * Z_CENTER / keys[index].z;
    canvas.DrawTextureW(texKeyMain,    screenPos, size, true, { .tint = colorPalette[overrideColorIndex][0] });
    canvas.DrawTextureW(texKeyHigh,    screenPos, size, true, { .tint = colorPalette[overrideColorIndex][1] });
    canvas.DrawTextureW(texKeyShadow,  screenPos, size, true, { .tint = colorPalette[overrideColorIndex][2] });
    canvas.DrawTextureW(texKeyOutline, screenPos, size);
}

void LimboApp::DrawKeys() {
    for (int i = 0; i < 8; i++) { // back keys
        if (keys[i].z < Z_CENTER) continue;
        DrawKey(i, 200, 0);
    }

    // canvas.Stroke(Math::fColor::White());
    // canvas.DrawText("Limbo", 216, { WIDTH * 0.3, HEIGHT * 0.7 }, { .rect = { WIDTH * 0.4, HEIGHT * 0.4 } });

    for (int i = 0; i < 8; i++) { // front keys
        if (keys[i].z >= Z_CENTER) continue;
        DrawKey(i, 200, 0);
    }
}

void LimboApp::ResetKeyPos() {
    for (int i = 0; i < 8; ++i) {
        const int j = keyPermutation[i];
        keys[j].position = TARGET_POSITIONS[i];
        keys[j].z = Z_CENTER;
    }
}

void LimboApp::LerpKeyPos() {
    for (int i = 0; i < 8; ++i) {
        const int j = keyPermutation[i];
        keys[j].position = keys[j].position.Lerp(TARGET_POSITIONS[i], 0.15f);
    }
}

Math::fv2 LimboApp::Project(Math::fv2 position, float z) {
    return ORIGIN + (position - ORIGIN) * (Z_CENTER / z);
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
        auto [x, y, z] = localX * curr.Cos() + localZ * curr.Sin() + origin;
        key.position = { x, y };
        key.z = z;
        curr += turn;
    }

    // canvas.DrawLine({ WIDTH * 0.5f, HEIGHT * 0.5f }, Project(mainAxis * fv3 { 300, 300, 10 } + origin));
}


LimboApp::ShufflePerm::ShufflePerm(Str permutation) {
    // perform standard permutation
    for (int i = 0; i < 8; i++) indices[i] = permutation[i] - 'A';
}

void LimboApp::ShufflePerm::Anim(LimboApp& app, float dt) {
    for (int i = 0; i < 8; ++i) {
        const int j = app.keyPermutation[indices[i]];
        app.keys[j].position = app.keys[j].position.Lerp(TARGET_POSITIONS[i], 0.15f);
    }
}

void LimboApp::ShufflePerm::Finish(LimboApp& app) {
    Permutation::Finish(app);
    Algorithm::ApplyPermutation(app.keyPermutation.AsSpan(), Spans::Vals(indices));
}

void LimboApp::RotatePerm::Anim(LimboApp& app, float dt) {
    const float lerpAngle = std::min((Math::PI - currentRotation) * 0.05f, Math::PI * dt * 2.0f);
    currentRotation += lerpAngle;
    const Math::Rotor2D rotDiff = Math::Radians(lerpAngle);
    for (auto& key : app.keys)
        key.position = ORIGIN + (key.position - ORIGIN).RotateBy(rotDiff);
}

void LimboApp::RotatePerm::Finish(LimboApp& app) {
    Permutation::Finish(app);
    app.keyPermutation.Reverse();
    app.ResetKeyPos();
}

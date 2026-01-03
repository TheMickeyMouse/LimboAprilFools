#include "LimboApp.h"
#include "Utils/Algorithm.h"

// A B C D
// E F G H
#define P_SHIFT_CW   "EABCFGHD"
#define P_SHIFT_CCW  "BCDHAEFG"
#define P_EASY_SHIFT "ECBHAGFD"
#define P_DIAG_SWAP  "FEHGBADC"
#define P_CYCLE_CELL_CW  { 1, 5, 6, 2, 0, 4, 7, 3 }
#define P_CYCLE_CELL_CCW { 4, 0, 3, 7, 5, 1, 2, 6 }
#define P_SWAP_CELLS     { 2, 3, 0, 1, 6, 7, 4, 5 }
#define P_REVERSE        { 7, 6, 5, 4, 3, 2, 1, 0 }
#define LEFT_CELLS  { 0, 1, 4, 5 }
#define RIGHT_CELLS { 2, 3, 6, 7 }

float Sigmoid(float x) {
    return 1.0f / (1.0f + std::exp(-x));
}

float Clamp(float x) {
    return std::clamp(x, 0.0f, 1.0f);
}

float CubicEase(float x) {
    x = Clamp(x);
    return x * x * (3.0f - 2.0f * x);
}

const Math::fv2 LimboApp::ORIGIN = { WIDTH / 2, HEIGHT / 2 };
const Math::fv2 LimboApp::TARGET_POSITIONS[8] = {
    { WIDTH * 0.2f, HEIGHT * 0.5f + WIDTH * 0.1f },
    { WIDTH * 0.4f, HEIGHT * 0.5f + WIDTH * 0.1f },
    { WIDTH * 0.6f, HEIGHT * 0.5f + WIDTH * 0.1f },
    { WIDTH * 0.8f, HEIGHT * 0.5f + WIDTH * 0.1f },
    { WIDTH * 0.2f, HEIGHT * 0.5f - WIDTH * 0.1f },
    { WIDTH * 0.4f, HEIGHT * 0.5f - WIDTH * 0.1f },
    { WIDTH * 0.6f, HEIGHT * 0.5f - WIDTH * 0.1f },
    { WIDTH * 0.8f, HEIGHT * 0.5f - WIDTH * 0.1f },
};

void LimboApp::Permutation::Anim(LimboApp& app, float dt) {
    time += dt / duration;
}

void LimboApp::Permutation::Finish(LimboApp& app) {
    app.ShuffleKeys(resultingPermutation);
}

LimboApp::LimboApp() : gdevice(Graphics::GraphicsDevice::Initialize({ (int)WIDTH, (int)HEIGHT }, { .transparent = true })) {
    if (ma_engine_init(nullptr, &audioEngine) != MA_SUCCESS) {
        Debug::QError$("Miniaudio Failed to Load!");
    }
    // const Graphics::TextureLoadParams params = { .pixelated = true };
    texKeyHigh    = Graphics::Texture2D::LoadPNG("../keyhighfix.png");
    texKeyMain    = Graphics::Texture2D::LoadPNG("../keymainfix.png");
    texKeyShadow  = Graphics::Texture2D::LoadPNG("../keyshadowfix.png");
    texKeyOutline = Graphics::Texture2D::LoadPNG("../keyoutline.png");
    texGlow       = Graphics::Texture2D::LoadPNG("../glow.png");
    Graphics::Image colorSrc = Graphics::Image::LoadPNG("../colorpalette.png");

    for (int i = 0; i < 8; ++i) {
        for (int tone = 0; tone < 3; ++tone) {
            // image is flipped
            colorPalette[i][tone] = (Math::fColor)colorSrc[{ tone, 7 - i }];
        }
    }

    ResetKeyPos();

    // A B C D
    // E F G H

    static constexpr float INV_SPEED = 1.0f;
    permutations = Vecs::New((Box<Permutation>[]) {
        Box<ShufflePerm>  ::Build("FGHCEABD",   0.28f * INV_SPEED),
        Box<CyclicPerm>   ::Build(false,        0.28f * INV_SPEED),
        Box<ShufflePerm>  ::Build(P_DIAG_SWAP,  0.28f * INV_SPEED),
        Box<CyclicPerm>   ::Build(true,         0.28f * INV_SPEED),
        Box<ShufflePerm>  ::Build("EGHCFABD",   0.28f * INV_SPEED),
        Box<DepthSwapPerm>::Build(true,         0.60f * INV_SPEED),
        Box<ShufflePerm>  ::Build("BCDAFGHE",   0.28f * INV_SPEED),
        Box<ShufflePerm>  ::Build("FGDHAEBC",   0.28f * INV_SPEED),
        Box<ShufflePerm>  ::Build("EAFHBCDG",   0.28f * INV_SPEED),
        Box<RotatePerm>   ::Build(false,        0.66f * INV_SPEED),
        Box<ShufflePerm>  ::Build(P_DIAG_SWAP,  0.28f * INV_SPEED),
        Box<ShufflePerm>  ::Build("BEDHAGFC",   0.28f * INV_SPEED),
        Box<ShufflePerm>  ::Build("BCDAFGHE",   0.25f * INV_SPEED),
        Box<ShufflePerm>  ::Build("DABCHEFG",   0.25f * INV_SPEED),
        Box<ShufflePerm>  ::Build(P_DIAG_SWAP,  0.25f * INV_SPEED),
        Box<ShufflePerm>  ::Build(P_EASY_SHIFT, 0.25f * INV_SPEED),
        Box<ShufflePerm>  ::Build("FGHCAEBD",   0.25f * INV_SPEED),
        Box<DepthSwapPerm>::Build(false,        0.50f * INV_SPEED),
        Box<RotatePerm>   ::Build(true,         0.45f * INV_SPEED),
        Box<CyclicPerm>   ::Build(true,         0.25f * INV_SPEED),
        Box<ShufflePerm>  ::Build(P_SHIFT_CCW,  0.25f * INV_SPEED),
        Box<ShufflePerm>  ::Build(P_DIAG_SWAP,  0.25f * INV_SPEED),
        Box<ShufflePerm>  ::Build(P_EASY_SHIFT, 0.25f * INV_SPEED),
        Box<ShufflePerm>  ::Build(P_DIAG_SWAP,  0.25f * INV_SPEED),
        Box<ShufflePerm>  ::Build(P_SHIFT_CW,   0.25f * INV_SPEED),
        Box<ShufflePerm>  ::Build(P_SHIFT_CW,   0.25f * INV_SPEED),
        Box<ShufflePerm>  ::Build(P_SHIFT_CCW,  0.25f * INV_SPEED),
    });

    Graphics::Render::UseBlendFunc(Graphics::BlendFactor::ONE, Graphics::BlendFactor::INVERT_SRC_ALPHA);

    ma_result result = ma_sound_init_from_file(&audioEngine, "../LimboMus.wav", 0, nullptr, nullptr, &music);
    if (result != MA_SUCCESS) {
        Debug::QError$("Failed to play Sound!");
    }
    static constexpr int SAMPLE_RATE = 44100;
    static constexpr int SKIP_FRAME_COUNT = (int)(10.28 * SAMPLE_RATE);
    ma_sound_seek_to_pcm_frame(&music, SKIP_FRAME_COUNT);
    ma_sound_start(&music);
}

LimboApp::~LimboApp() {
    ma_engine_uninit(&audioEngine);
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

    if (io.Keyboard.KeyOnPress(IO::Key::C)) {
        showColors = !showColors;
    }

    if (!currentPerm || (frame < permutations.Length() && currentPerm->Done())) {
        if (currentPerm) {
            currentPerm->Finish(*this);
        }
        ResetKeyPos();
        currentPerm = permutations[frame].AsRef();
        ++frame;
    }

    canvas.EndFrame();
    gdevice.End();
    return true;
}

void LimboApp::DrawKey(Math::fv2 pos, float scale, int colorIndex) {
    canvas.transform = Math::Transform2D(pos, 1, Math::Radians(globalRotation));
    canvas.DrawTextureW(texKeyMain,    0, scale, true, { .tint = colorPalette[colorIndex][0] });
    canvas.DrawTextureW(texKeyHigh,    0, scale, true, { .tint = colorPalette[colorIndex][1] });
    canvas.DrawTextureW(texKeyShadow,  0, scale, true, { .tint = colorPalette[colorIndex][2] });
    canvas.DrawTextureW(texKeyOutline, 0, scale);
    canvas.DrawTextureW(texGlow,       0, scale, true, { .tint = colorPalette[colorIndex][0].AddAlpha(0.2f) });
    canvas.transform.Reset();
}

void LimboApp::DrawKey(int index, float scale, int overrideColorIndex) {
    overrideColorIndex = overrideColorIndex == -1 ? index : overrideColorIndex;
    const Math::fv2 screenPos = Project(keys[index].position, keys[index].z);
    const float size = scale * Z_CENTER / keys[index].z;
    DrawKey(screenPos, size, overrideColorIndex);
}

void LimboApp::DrawKeys() {
    for (int i = 0; i < 8; i++) { // back keys
        if (keys[i].z < Z_CENTER) continue;
        DrawKey(i, KEY_SIZE, showColors * i);
    }

    // canvas.Stroke(Math::fColor::White());
    // canvas.DrawText("Limbo", 216, { WIDTH * 0.3, HEIGHT * 0.7 }, { .rect = { WIDTH * 0.4, HEIGHT * 0.4 } });

    for (int i = 0; i < 8; i++) { // front keys
        if (keys[i].z >= Z_CENTER) continue;
        DrawKey(i, KEY_SIZE, showColors * i);
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
        keys[j].position.LerpToward(TARGET_POSITIONS[i], 0.1f);
    }
}

void LimboApp::ShuffleKeys(Span<int> indices) {
    Algorithm::ApplyRevPermutationInPlace(keyPermutation.AsSpan(), indices);
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


LimboApp::ShufflePerm::ShufflePerm(Str permutation, float dura) : Permutation(dura) {
    // perform standard permutation
    for (int i = 0; i < 8; i++) resultingPermutation[i] = permutation[i] - 'A';

}

void LimboApp::ShufflePerm::Anim(LimboApp& app, float dt) {
    Permutation::Anim(app, dt);
    const float s = Sigmoid(10.0f * (time - 0.5f));
    for (int i = 0; i < 8; ++i) {
        const int j = resultingPermutation[i];
        app.keys[app.keyPermutation[j]].position = TARGET_POSITIONS[j].Lerp(TARGET_POSITIONS[i], s);
    }
}

LimboApp::CyclicPerm::CyclicPerm(bool ccw, float dura) : Permutation(dura), ccw(ccw) {
    if (ccw) {
        resultingPermutation = P_CYCLE_CELL_CCW;
    } else {
        resultingPermutation = P_CYCLE_CELL_CW;
    }
}

void LimboApp::CyclicPerm::Anim(LimboApp& app, float dt) {
    Permutation::Anim(app, dt);
    const float angle = CubicEase(time) * Math::HALF_PI;
    const Math::Rotor2D rotation = Math::Radians(-f32s::Signed(ccw, angle));

    const Math::fv2 L_ORIGIN = { WIDTH * 0.3f, HEIGHT * 0.5f };
    for (int i : LEFT_CELLS) {
        auto& key = app.keys[app.keyPermutation[i]];
        key.position = L_ORIGIN + (TARGET_POSITIONS[i] - L_ORIGIN).RotateBy(rotation);
    }
    const Math::fv2 R_ORIGIN = { WIDTH * 0.7f, HEIGHT * 0.5f };
    for (int i : RIGHT_CELLS) {
        auto& key = app.keys[app.keyPermutation[i]];
        key.position = R_ORIGIN + (TARGET_POSITIONS[i] - R_ORIGIN).RotateBy(-rotation);
    }
}

LimboApp::RotatePerm::RotatePerm(bool reverse, float dura) : Permutation(dura), reverse(reverse) {
    resultingPermutation = P_REVERSE;
}

void LimboApp::RotatePerm::Anim(LimboApp& app, float dt) {
    Permutation::Anim(app, dt);
    const float angle = CubicEase(time) * Math::PI;
    const Math::Rotor2D rotDiff = Math::Radians(-angle);

    app.globalRotation -= angle - currentAngle;
    currentAngle = angle;
    const Math::fv2 ROTATION_ORIGIN = { WIDTH * 0.5f, HEIGHT * 0.5f };

    const float s = Sigmoid(10.0f * (time - 0.5f));
    for (int i = 0; i < 8; ++i) {
        auto& key = app.keys[app.keyPermutation[i]];
        if (i % 4 == (reverse ? 0 : 3)) {
            key.position = TARGET_POSITIONS[i].Lerp(TARGET_POSITIONS[(i + (reverse ? 7 : 1)) % 8], s);
        } else {
            key.position = ROTATION_ORIGIN + (TARGET_POSITIONS[i] - ROTATION_ORIGIN).RotateBy(rotDiff);
        }
        // key.z = z;
    }
}

LimboApp::DepthSwapPerm::DepthSwapPerm(bool reverse, float dura) : Permutation(dura), reverse(reverse) {
    resultingPermutation = P_SWAP_CELLS;
}

void LimboApp::DepthSwapPerm::Anim(LimboApp& app, float dt) {
    Permutation::Anim(app, dt);
    const float dx = WIDTH * 0.4f * Sigmoid(20 * time - 8),
                dz = Sigmoid(20 * time - 3) - Sigmoid(20 * time - 16);
    for (int i = 0; i < 8; ++i) {
        auto& key = app.keys[app.keyPermutation[i]];
        key.z = Z_CENTER * (1 + ((i % 4 < 2) == reverse ? -0.2f : 0.5f) * dz);
        key.position = TARGET_POSITIONS[i] + Math::fv2 { (i % 4 < 2 ? dx : -dx), 0 };
    }
}

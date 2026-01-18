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
#define RES "../res/"
    Graphics::Image colorSrc = Graphics::Image::LoadPNG(RES"colorpalette.png");

    texAtlas = Graphics::TextureAtlas::FromFiles(
        {{ RES"keyhighfix.png", RES"keymain.png", RES"keyshadowfix.png", RES"keyoutline.png", RES"glow.png", RES"ready.png", RES"1.png", RES"2.png", RES"3.png", RES"go.png" }},
        {{ "high", "main", "shadow", "outline", "glow", "ready", "1", "2", "3", "go" }},
        true
    );

    for (int i = 0; i < 8; ++i) {
        for (int tone = 0; tone < 3; ++tone) {
            // image is flipped
            colorPalette[i][tone] = (Math::fColor)colorSrc[{ tone, 7 - i }];
            keys[i].color[tone] = colorPalette[0][tone];
        }
    }

    ResetKeyPos();

    // A B C D
    // E F G H

    Math::RandomGenerator rand;
    static constexpr int VALID_PERMUTATIONS[] = {
        0x0123, 0x0126, 0x0154, 0x0156, 0x0456, 0x0451,
        0x1045, 0x1237, 0x1265, 0x1267, 0x1540, 0x1567,
        0x1562, 0x2104, 0x2154, 0x2156, 0x2376, 0x2654,
        0x2651, 0x2673, 0x3210, 0x3215, 0x3265, 0x3267,
        0x3765, 0x3762, 0x4012, 0x4015, 0x4567, 0x4562,
        0x4510, 0x4512, 0x5401, 0x5673, 0x5621, 0x5623,
        0x5104, 0x5123, 0x5126, 0x6540, 0x6510, 0x6512,
        0x6732, 0x6210, 0x6215, 0x6237, 0x7654, 0x7651,
        0x7621, 0x7623, 0x7321, 0x7326
    };
    const int STARTING_PERM = rand.Choose(Span(VALID_PERMUTATIONS));
    const int keyPos[4] = { STARTING_PERM & 0xF, (STARTING_PERM & 0xF0) >> 4, (STARTING_PERM & 0xF00) >> 8, (STARTING_PERM & 0xF000) >> 12 };
    CArray<char, 9> permString[3] = { "ABCDEFGH", "ABCDEFGH", "ABCDEFGH" };
    std::swap(permString[0][keyPos[0]], permString[0][keyPos[1]]);
    std::swap(permString[1][keyPos[1]], permString[1][keyPos[2]]);
    std::swap(permString[2][keyPos[2]], permString[2][keyPos[3]]);

    static constexpr float INV_SPEED = 1.0f;
    permutations = Vecs::New((Box<Permutation>[]) {
        Box<GlowAnim>     ::Build(keyPos[0], 3,    3.30f * INV_SPEED),
        Box<ShufflePerm>  ::Build(permString[0],   0.56f * INV_SPEED),
        Box<ShufflePerm>  ::Build(permString[1],   0.56f * INV_SPEED),
        Box<ShufflePerm>  ::Build(permString[2],   0.56f * INV_SPEED),
        Box<Permutation>  ::Build(                 0.54f * INV_SPEED),
        Box<GlowAnim>     ::Build(keyPos[3], 2,    2.20f * INV_SPEED),
        Box<ReadyAnim>    ::Build(                 1.65f * INV_SPEED),
        Box<ShufflePerm>  ::Build("FGHCEABD",      0.28f * INV_SPEED),
        Box<CyclicPerm>   ::Build(false,           0.28f * INV_SPEED),
        Box<ShufflePerm>  ::Build(P_DIAG_SWAP,     0.30f * INV_SPEED),
        Box<CyclicPerm>   ::Build(true,            0.28f * INV_SPEED),
        Box<ShufflePerm>  ::Build("EGHCFABD",      0.28f * INV_SPEED),
        Box<DepthSwapPerm>::Build(true,            0.82f * INV_SPEED),
        Box<ShufflePerm>  ::Build("BCDAFGHE",      0.28f * INV_SPEED),
        Box<ShufflePerm>  ::Build("FGDHAEBC",      0.28f * INV_SPEED),
        Box<ShufflePerm>  ::Build("EAFHBCDG",      0.28f * INV_SPEED),
        Box<RotatePerm>   ::Build(false,           0.66f * INV_SPEED),
        Box<ShufflePerm>  ::Build(P_DIAG_SWAP,     0.28f * INV_SPEED),
        Box<ShufflePerm>  ::Build("BEDHAGFC",      0.28f * INV_SPEED),
        Box<ShufflePerm>  ::Build("BCDAFGHE",      0.28f * INV_SPEED),
        Box<ShufflePerm>  ::Build("DABCHEFG",      0.28f * INV_SPEED),
        Box<ShufflePerm>  ::Build(P_DIAG_SWAP,     0.28f * INV_SPEED),
        Box<ShufflePerm>  ::Build(P_EASY_SHIFT,    0.28f * INV_SPEED),
        Box<ShufflePerm>  ::Build("FGHCAEBD",      0.28f * INV_SPEED),
        Box<DepthSwapPerm>::Build(false,           0.50f * INV_SPEED),
        Box<RotatePerm>   ::Build(true,            0.45f * INV_SPEED),
        Box<CyclicPerm>   ::Build(true,            0.25f * INV_SPEED),
        Box<ShufflePerm>  ::Build(P_SHIFT_CCW,     0.25f * INV_SPEED),
        Box<ShufflePerm>  ::Build(P_DIAG_SWAP,     0.25f * INV_SPEED),
        Box<ShufflePerm>  ::Build(P_EASY_SHIFT,    0.25f * INV_SPEED),
        Box<ShufflePerm>  ::Build("EFGHABCD",      0.25f * INV_SPEED),
        Box<ShufflePerm>  ::Build(P_SHIFT_CW,      0.25f * INV_SPEED),
        Box<ShufflePerm>  ::Build(P_SHIFT_CW,      0.25f * INV_SPEED),
        Box<ShufflePerm>  ::Build(P_SHIFT_CCW,     0.25f * INV_SPEED),
        Box<EndAnim>      ::Build(                 1.00f * INV_SPEED),
    });

    Graphics::Render::UseBlendFunc(Graphics::BlendFactor::ONE, Graphics::BlendFactor::INVERT_SRC_ALPHA);

    ma_result result = ma_sound_init_from_file(&audioEngine, RES"LimboMus.wav", 0, nullptr, nullptr, &music);
    if (result != MA_SUCCESS) {
        Debug::QError$("Failed to play Sound!");
    }
    // static constexpr int SAMPLE_RATE = 44100;
    // const float animTime = permutations.Iter().Map([] (const Box<Permutation>& p) {
    //                            return p->duration;
    //                        }).Sum().Unwrap() / INV_SPEED;
    // int SKIP_FRAME_COUNT = (int)((18.54 - animTime) * SAMPLE_RATE);
    // ma_sound_seek_to_pcm_frame(&music, SKIP_FRAME_COUNT);
    ma_sound_set_pitch(&music, 1.0f / INV_SPEED);
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
    if (currentPerm)
        currentPerm->LateAnim(*this, io.Time.DeltaTime());

    // jailbreak safety measure
    if (io.Keyboard.KeyOnPress(IO::Key::F)) {
        return false;
    }

    if (io.Keyboard.KeyOnPress(IO::Key::C)) {
        showColors = !showColors;
    }

    if (!currentPerm || (frame < permutations.Length() && currentPerm->Done())) {
        const float extraTime = currentPerm ? currentPerm->ExtraTime() : 0;
        if (currentPerm) {
            currentPerm->Finish(*this);
        }
        ResetKeyPos();
        currentPerm = permutations[frame].AsRef();
        currentPerm->AddTime(extraTime);
        ++frame;
    }

    canvas.EndFrame();
    gdevice.End();
    return true;
}

void LimboApp::DrawKey(Math::fv2 pos, float scale, const Math::fColor palette[3]) {
    canvas.transform = Math::Transform2D(pos, 1, Math::Radians(globalRotation));
    canvas.DrawSTextureW(texAtlas["main"],    0, scale, true, palette[0]);
    canvas.DrawSTextureW(texAtlas["high"],    0, scale, true, palette[1]);
    canvas.DrawSTextureW(texAtlas["shadow"],  0, scale, true, palette[2]);
    canvas.DrawSTextureW(texAtlas["outline"], 0, scale);
    canvas.DrawSTextureW(texAtlas["glow"],    0, scale, true, palette[0].AddAlpha(0.25f));
    canvas.transform.Reset();
}

void LimboApp::DrawKey(Math::fv2 pos, float scale, int colorIndex) {
    DrawKey(pos, scale, colorPalette[colorIndex]);
}

void LimboApp::DrawKey(int index, float scale, int overrideColorIndex) {
    const Math::fv2 screenPos = Project(keys[index].position, keys[index].z);
    const float size = scale * Z_CENTER / keys[index].z;
    DrawKey(screenPos, size, overrideColorIndex != -1 ? GetColorShades(overrideColorIndex) : keys[index].color);
}

void LimboApp::DrawKeys() {
    for (int i = 0; i < 8; i++) { // back keys
        if (keys[i].z < Z_CENTER) continue;
        DrawKey(i, KEY_SIZE, showColors ? i : -1);
    }

    // canvas.Stroke(Math::fColor::White());
    // canvas.DrawText("Limbo", 216, { WIDTH * 0.3, HEIGHT * 0.7 }, { .rect = { WIDTH * 0.4, HEIGHT * 0.4 } });

    for (int i = 0; i < 8; i++) { // front keys
        if (keys[i].z >= Z_CENTER) continue;
        DrawKey(i, KEY_SIZE, showColors ? i : -1);
    }
}

const Math::fColor& LimboApp::GetColor(int index, int shade) const {
    return colorPalette[index][shade];
}

const CArray<Math::fColor, 3>& LimboApp::GetColorShades(int index) const {
    return colorPalette[index];
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
    const float dx = WIDTH * 0.4f * Sigmoid(20 * time - 10),
                dz = Sigmoid(30 * time - 6) - Sigmoid(30 * time - 26);
    for (int i = 0; i < 8; ++i) {
        auto& key = app.keys[app.keyPermutation[i]];
        key.z = Z_CENTER * (1 + ((i % 4 < 2) == reverse ? -0.2f : 0.5f) * dz);
        key.position = TARGET_POSITIONS[i] + Math::fv2 { (i % 4 < 2 ? dx : -dx), 0 };
    }
}

LimboApp::GlowAnim::GlowAnim(int glowingKey, int flashCount, float dura) : Permutation(dura), glowingKey(glowingKey), flashCount(flashCount) {}

void LimboApp::GlowAnim::Anim(LimboApp& app, float dt) {
    Permutation::Anim(app, dt);
    auto& key = app.keys[app.keyPermutation[glowingKey]];
    float s = 0.5f + 0.5f * std::cos(2.0f * flashCount * time * Math::PI);
    s = 1 - s * s;
    for (int i = 0; i < 3; ++i) {
        key.color[i] = app.colorPalette[0][i].Lerp(app.colorPalette[3][i], s);
    }
}

void LimboApp::GlowAnim::Finish(LimboApp& app) {
    Permutation::Finish(app);
}

void LimboApp::ReadyAnim::LateAnim(LimboApp& app, float dt) {
    Permutation::LateAnim(app, dt);
    static constexpr float ACC_TIMES[] = { 0.0f, 0.33f, 0.5f, 0.67f, 0.83f, 1.0f };
    static Str TEXTURES[] = { "ready", "3", "2", "1", "go" };
    const int texIndex = (int)(Span { ACC_TIMES }.FindIf([&] (float x) { return x > time; }).UnwrapOr(5));
    const float t = time - ACC_TIMES[texIndex - 1], dur = ACC_TIMES[texIndex] - ACC_TIMES[texIndex - 1];
    const Graphics::SubTexture tex = app.texAtlas[TEXTURES[texIndex - 1]];

    if (texIndex == 5) {
        const float y = CubicEase(t * 18), alpha = CubicEase((t - (dur - 0.05f)) * 18);
        app.canvas.DrawSTextureH(tex, { WIDTH * 0.5f, HEIGHT * y * 0.5f }, WIDTH * 0.15f * std::exp(alpha), true, { 1, 1 - alpha });
        return;
    }
    float y;
    if (texIndex == 1) {
        const float recoilT = std::max((t - (dur - 0.1f)) * 18, -0.05f * 18);
        y = CubicEase(t * 18) + (recoilT * recoilT) - 0.81f;
    } else {
        y = CubicEase(t * 18) + CubicEase((t - (dur - 0.05f)) * 18);
    }

    // app.canvas.transform = { { WIDTH * 0.5f, HEIGHT * y * 0.5f }, 1,  };
    app.canvas.DrawSTextureH(tex, { WIDTH * 0.5f, HEIGHT * y * 0.5f }, WIDTH * 0.15f);
}

LimboApp::EndAnim::EndAnim(float dura, LimboApp& app) : Permutation(dura) {
    time = 1.3f;

    // app.canvas.AddInteractable()
}

void LimboApp::EndAnim::Anim(LimboApp& app, float dt) {
    // dont use this: Permutation::Anim(app, dt);
    time += dt;

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

    static constexpr int REV_PERM[] = { 3, 2, 1, 0, 4, 5, 6, 7 };
    for (int i = 0; i < 8; ++i) {
        auto& key = app.keys[app.keyPermutation[REV_PERM[i]]];
        auto [x, y, z] = localX * curr.Cos() + localZ * curr.Sin() + origin;
        key.position.LerpToward({ x, y }, 0.05f);
        key.z = std::lerp(key.z, z, 0.05f);
        curr += turn;
        for (int tone = 0; tone < 3; ++tone) {
            key.color[tone].LerpTowards(app.GetColor(app.keyPermutation[i], tone), 0.2f);
        }
    }
}

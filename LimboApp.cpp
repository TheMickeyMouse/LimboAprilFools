#include "LimboApp.h"

#include "System.h"
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

#define RES "../res/"

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

float EaseOutBack(float x, float c = 1.70158) {
    x = Clamp(x) - 1;
    return 1 + ((c + 1) * x + c) * x * x;
}

Palette Palette::operator*(float x) const {
    return { operator[](0) * x, operator[](1) * x, operator[](2) * x };
}

void Palette::LerpTowards(const Palette& p, float t) {
    operator[](0).LerpTowards(p[0], t);
    operator[](1).LerpTowards(p[1], t);
    operator[](2).LerpTowards(p[2], t);
}

Palette Palette::Lerp(const Palette& p, float t) const {
    return { operator[](0).Lerp(p[0], t), operator[](1).Lerp(p[1], t), operator[](2).Lerp(p[2], t) };
}

void ScreenShake::Trigger(float amp) {
    amplitude = amp;
}
void ScreenShake::Update(float dt) {
    amplitude = std::max(amplitude - 240.0f * dt, 0.0f);
    if (amplitude != 0.0f) {
        auto& rand = GraphicsDevice::GetDeviceInstance().GetRand();
        offset = fv2::RandomInUnit(rand) * amplitude;
    }
}

void LimboApp::Permutation::Finish(LimboApp& app) {
    app.ShuffleKeys(resultingPermutation);
    app.ResetKeyPos();
}

LimboApp::Intensify::Intensify(GraphicsDevice& gdevice)
    : Effect(9.65f),
      postEffect({ (int)WIDTH, (int)HEIGHT }, Shader::FromFile(RES"post.glsl")),
      background(Texture2D::New(System::CaptureScreen(), {
          .format = TextureFormat::BGRA, .pixelated = true, .border = TextureBorder::CLAMP_TO_BORDER })),
      temp(Texture2D::New(nullptr, { (int)WIDTH, (int)HEIGHT })) {}

void LimboApp::Intensify::Anim(LimboApp& app, float dt) {
    if (state == MANUAL || state == CAPTURE || state == DISABLED) {
        // Debug::QInfo$("time = {}", time);
        return;
    }
    if (state == NO_ANIM) {
        app.globalScale = std::lerp(app.globalScale, 1.0f, 0.05f);
        return;
    }

    Effect::Anim(app, dt);
    if (state == USE_EXPOSURE) return;

    const float t = std::min(time, 1.0f);

    innerRadius    = std::lerp(1.1f, 0.0f, t);
    outerRadius    = std::lerp(1.4f, 1.0f, t);
    vignetteTint.a = std::lerp(0.0f, 0.95f, t);
    aberrationOff  = { (3 + 10.0f * t) / WIDTH, (-2 - 6.0f * t) / HEIGHT };

    app.globalScale = std::lerp(1.0f, 1.2f, t);
    app.screenShake.Trigger(std::exp(16 * t - 15) * 75.0f);
}

void LimboApp::Intensify::Reset(LimboApp& app) {
    state = NO_ANIM;
    time = 0;
    aberrationOff = { 3 / WIDTH, -2 / HEIGHT };
}

void LimboApp::Intensify::EnterExposure(LimboApp& app) {
    state = CAPTURE;
}

void LimboApp::Intensify::Use() {
    if (state == DISABLED) return;
    postEffect.SetToRenderTarget();
}

void LimboApp::Intensify::Draw() {
    if (state == DISABLED) return;
    if (state == USE_EXPOSURE) {
        postEffect.shader.Bind();
        postEffect.shader.SetUniformFloat("brightness", std::lerp(1.0f, 10.0f, 3.0f * Clamp(time)));
        postEffect.shader.SetUniformFloat("contrast",   std::lerp(0.0f, 1.0f,  3.0f * Clamp(time)));
        postEffect.shader.SetUniformTex  ("background", background, 0);
        postEffect.ApplyEffect();
        return;
    }
    postEffect.shader.Bind();
    postEffect.shader.SetUniformFloat("innerRadius",   innerRadius);
    postEffect.shader.SetUniformFloat("outerRadius",   outerRadius);
    postEffect.shader.SetUniformColor("vignetteTint",  vignetteTint);
    postEffect.shader.SetUniformFv2  ("aberrationOff", aberrationOff);
    postEffect.shader.SetUniformInt  ("vignetteOver",  vignetteForeground);
    postEffect.shader.SetUniformTex  ("background",    background, 0);
    postEffect.ApplyEffect();
    if (state == CAPTURE) {
        postEffect.frameBuf.Bind();
        postEffect.frameBuf.Attach(background);
        postEffect.frameBuf.BlitFromScreen({ 0, { (int)WIDTH, (int)HEIGHT } }, { 0, { (int)WIDTH, (int)HEIGHT } });
        postEffect.frameBuf.Bind();
        postEffect.frameBuf.Attach(postEffect.screenTex);
        postEffect.shader = Shader::FromFile(RES"exposure.glsl");
        time = -0.2f;
        state = USE_EXPOSURE;
    }
}

const fv2 LimboApp::ORIGIN = { WIDTH / 2, HEIGHT / 2 };

const fv2 LimboApp::TARGET_POSITIONS[8] = {
    { WIDTH * 0.2f, HEIGHT * 0.5f + WIDTH * 0.1f },
    { WIDTH * 0.4f, HEIGHT * 0.5f + WIDTH * 0.1f },
    { WIDTH * 0.6f, HEIGHT * 0.5f + WIDTH * 0.1f },
    { WIDTH * 0.8f, HEIGHT * 0.5f + WIDTH * 0.1f },
    { WIDTH * 0.2f, HEIGHT * 0.5f - WIDTH * 0.1f },
    { WIDTH * 0.4f, HEIGHT * 0.5f - WIDTH * 0.1f },
    { WIDTH * 0.6f, HEIGHT * 0.5f - WIDTH * 0.1f },
    { WIDTH * 0.8f, HEIGHT * 0.5f - WIDTH * 0.1f },
};

LimboApp::LimboApp() : gdevice(GraphicsDevice::Initialize({ (int)WIDTH, (int)HEIGHT }, { .fullscreen = true })) {
    if (ma_engine_init(nullptr, &audioEngine) != MA_SUCCESS) {
        Debug::QError$("Miniaudio Failed to Load!");
    }
    // const TextureLoadParams params = { .pixelated = true };
    Image colorSrc = Image::LoadPNG(RES"colorpalette.png");

    texAtlas = TextureAtlas::FromFiles(
        { { RES"keyhigh.png", RES"keymain.png", RES"keyshadow.png", RES"keyoutline.png",
            RES"glow.png", RES"ready.png", RES"1.png", RES"2.png", RES"3.png", RES"go.png",
            RES"ominous_hands.png", RES"spotlight.png", RES"icons.png", RES"choose.png",
            RES"correct.png", RES"wrong.png", RES"fake_error.png",
            RES"computer.png", RES"comp_bg.png", RES"bsod.png", RES"ur_comp.png" } },

        { { "high", "main", "shadow", "outline", "glow", "ready", "1", "2", "3", "go",
            "hands", "light", "icons", "choose", "correct", "wrong", "err", "comp", "comp_bg",
            "bsod", "ur_comp" } },
        true
    );

    for (int i = 0; i < 8; ++i) {
        for (int tone = 0; tone < 3; ++tone) {
            // image is flipped
            colorPalette[i][tone] = (fColor)colorSrc[{ tone, 7 - i }];
        }
        keys[i].colors = colorPalette[0];
    }

    ResetKeyPos();

    // A B C D
    // E F G H

    RandomGenerator rand;
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
    correctKey = keys[keyPos[0]];

    static constexpr float INV_SPEED = 1.0f;

    timeline = Timeline {
        Vecs::New((Box<Effect>[]) {
            Boxs::New(GlowAnim      { correctKey, 3, 3.30f * INV_SPEED }),
            Boxs::New(ShufflePerm   { permString[0], 0.56f * INV_SPEED }),
            Boxs::New(ShufflePerm   { permString[1], 0.56f * INV_SPEED }),
            Boxs::New(ShufflePerm   { permString[2], 0.56f * INV_SPEED }),
            Boxs::New(ShufflePerm   { "ABCDEFGH",    0.54f * INV_SPEED }),
            Boxs::New(GlowAnim      { correctKey, 2, 2.20f * INV_SPEED }),
            Boxs::New(ReadyAnim     {                1.65f * INV_SPEED }),
            Boxs::New(ShufflePerm   { "FGHCEABD",    0.28f * INV_SPEED }),
            Boxs::New(CyclicPerm    { false,         0.28f * INV_SPEED }),
            Boxs::New(ShufflePerm   { P_DIAG_SWAP,   0.30f * INV_SPEED }),
            Boxs::New(CyclicPerm    { true,          0.28f * INV_SPEED }),
            Boxs::New(ShufflePerm   { "EGHCFABD",    0.28f * INV_SPEED }),
            Boxs::New(DepthSwapPerm { true,          0.82f * INV_SPEED }),
            Boxs::New(ShufflePerm   { "BCDAFGHE",    0.28f * INV_SPEED }),
            Boxs::New(ShufflePerm   { "FGDHAEBC",    0.28f * INV_SPEED }),
            Boxs::New(ShufflePerm   { "EAFHBCDG",    0.28f * INV_SPEED }),
            Boxs::New(RotatePerm    { false,         0.66f * INV_SPEED }),
            Boxs::New(ShufflePerm   { P_DIAG_SWAP,   0.28f * INV_SPEED }),
            Boxs::New(ShufflePerm   { "BEDHAGFC",    0.28f * INV_SPEED }),
            Boxs::New(ShufflePerm   { "BCDAFGHE",    0.28f * INV_SPEED }),
            Boxs::New(ShufflePerm   { "DABCHEFG",    0.28f * INV_SPEED }),
            Boxs::New(ShufflePerm   { P_DIAG_SWAP,   0.28f * INV_SPEED }),
            Boxs::New(ShufflePerm   { P_EASY_SHIFT,  0.28f * INV_SPEED }),
            Boxs::New(ShufflePerm   { "FGHCAEBD",    0.28f * INV_SPEED }),
            Boxs::New(DepthSwapPerm { false,         0.50f * INV_SPEED }),
            Boxs::New(RotatePerm    { true,          0.45f * INV_SPEED }),
            Boxs::New(CyclicPerm    { true,          0.25f * INV_SPEED }),
            Boxs::New(ShufflePerm   { P_SHIFT_CCW,   0.25f * INV_SPEED }),
            Boxs::New(ShufflePerm   { P_DIAG_SWAP,   0.25f * INV_SPEED }),
            Boxs::New(ShufflePerm   { P_EASY_SHIFT,  0.25f * INV_SPEED }),
            Boxs::New(ShufflePerm   { "EFGHABCD",    0.25f * INV_SPEED }),
            Boxs::New(ShufflePerm   { P_SHIFT_CW,    0.25f * INV_SPEED }),
            Boxs::New(ShufflePerm   { P_SHIFT_CW,    0.25f * INV_SPEED }),
            Boxs::New(ShufflePerm   { P_SHIFT_CCW,   0.25f * INV_SPEED }),
            Boxs::New(ChooseKeyAnim {                1.00f * INV_SPEED }),
            Boxs::New(EndAnim       {                8.00f * INV_SPEED }),
            Boxs::New(Finish        { 0 }),
        })
    };

    Render::UseBlendFunc(BlendFactor::ONE, BlendFactor::INVERT_SRC_ALPHA);


    // static constexpr int SAMPLE_RATE = 44100;
    Debug::QInfo$("Total Anim Time: {}", timeline.totalDuration);
    // int SKIP_FRAME_COUNT = (int)((18.54 - animTime) * SAMPLE_RATE);
    // ma_sound_seek_to_pcm_frame(&music, SKIP_FRAME_COUNT);
    intensify = { gdevice };

#if 0
    ma_sound_init_from_file(&audioEngine, RES"LimboMus.mp3",  0, nullptr, nullptr, &music);
    ma_sound_set_pitch(&music, 1.0f / INV_SPEED);
    ma_sound_set_volume(&music, 0.2f);
    ma_sound_start(&music);
#endif
}

LimboApp::~LimboApp() {
    ma_engine_uninit(&audioEngine);

    // System::ShowTaskbar();
}

bool LimboApp::Run() {
    if (finished) return false;

    intensify.Use();

    gdevice.Begin();
    canvas.BeginFrame();

    // canvas.Fill(fColor::White());
    // canvas.DrawRect({ { WIDTH * 0.05, HEIGHT * 0.15 }, { WIDTH * 0.95, HEIGHT * 0.85 } });
    // canvas.DrawText("Hello, World!", 60, { 560, 740 }, { .rect = { 800, 400 } });
    const auto& io = gdevice.GetIO();
    const float dt = io.Time.DeltaTime();

    // SetSpinningKeys();
    timeline.Anim(*this, dt);

    // jailbreak safety measure
    if (io.Keyboard.KeyOnPress(IO::Key::F)) {
        return false;
    }

    if (io.Keyboard.KeyOnPress(IO::Key::H)) {
        showHitboxes = !showHitboxes;
    }
    if (showHitboxes)
        canvas.ShowHitboxes();

    canvas.Update(dt);
    screenShake.Update(dt);
    intensify.Anim(*this, dt);

    canvas.Stroke(1);
    canvas.DrawText(Text::Format("FPS = {}", (int)io.Time.Framerate()), 30.0f, { 0, HEIGHT },
        { .alignment = TextAlign::LEFT | TextAlign::VTOP });

    canvas.EndFrame();
    intensify.Draw();

    gdevice.End();
    return true;
}

fv2 LimboApp::Project(fv2 position, float z) {
    return ORIGIN + (position - ORIGIN) * (Z_CENTER / z);
}

void LimboApp::DrawKey(int index) {
    LimboKey& key = keys[index];
    const fv2 screenPos = Project(key.position, key.z / globalScale);
    const float size = globalScale * key.scale * KEY_SIZE * Z_CENTER / key.z;
    const auto& palette = key.colors;
    canvas.transform = Transform2D(screenPos + screenShake.offset, 1, Radians(globalRotation));
    canvas.DrawSTextureW(texAtlas["main"],    0, size, true, palette[0]);
    canvas.DrawSTextureW(texAtlas["high"],    0, size, true, palette[1]);
    canvas.DrawSTextureW(texAtlas["shadow"],  0, size, true, palette[2]);
    canvas.DrawSTextureW(texAtlas["outline"], 0, size);
    canvas.DrawSTextureW(texAtlas["glow"],    0, size * 1.25f, true, palette[0].AddAlpha(key.glowIntensity));
    canvas.transform.Reset();
}

void LimboApp::DrawFrontKeys() {
    for (int i = 0; i < 8; i++) {
        if (keys[i].z >= Z_CENTER) continue;
        DrawKey(i);
    }
}

void LimboApp::DrawBackKeys() {
    for (int i = 0; i < 8; i++) {
        if (keys[i].z < Z_CENTER) continue;
        DrawKey(i);
    }
}

void LimboApp::DrawKeys() {
    DrawBackKeys();
    DrawFrontKeys();
}

void LimboApp::DrawTexW(Str name, const fv2& pos, float w, float alpha) {
    canvas.DrawSTextureW(texAtlas[name], pos, w, true, { 1, alpha });
}

void LimboApp::DrawTexH(Str name, const fv2& pos, float h, float alpha) {
    canvas.DrawSTextureH(texAtlas[name], pos, h, true, { 1, alpha });
}

void LimboApp::DrawTexHR(Str name, const fv2& pos, float h, float alpha, float angle) {
    canvas.transform = { pos, 1, Radians(angle) };
    canvas.DrawSTextureH(texAtlas[name], 0, h, true, { 1, alpha });
    canvas.transform.Reset();
}

const fColor& LimboApp::GetColor(int index, int shade) const {
    return colorPalette[index][shade];
}

const Palette& LimboApp::GetColorShades(int index) const {
    return colorPalette[index];
}

const LimboKey& LimboApp::KeyAt(int index) const {
    return keys[keyPermutation[index]];
}

LimboKey& LimboApp::KeyAt(int index) {
    return keys[keyPermutation[index]];
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

LimboApp::ShufflePerm::ShufflePerm(Str permutation, float dura) : Permutation(dura) {
    // perform standard permutation
    for (int i = 0; i < 8; i++) resultingPermutation[i] = permutation[i] - 'A';
}

void LimboApp::ShufflePerm::Anim(LimboApp& app, float dt) {
    Permutation::Anim(app, dt);
    const float s = Sigmoid(10.0f * (time - 0.5f));
    for (int i = 0; i < 8; ++i) {
        const int j = resultingPermutation[i];
        app.KeyAt(j).position = TARGET_POSITIONS[j].Lerp(TARGET_POSITIONS[i], s);
    }

    app.DrawKeys();
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
    const float angle = CubicEase(time) * HALF_PI;
    const Rotor2D rotation = Radians(-f32s::Signed(ccw, angle));

    const fv2 L_ORIGIN = { WIDTH * 0.3f, HEIGHT * 0.5f };
    for (int i : LEFT_CELLS) {
        auto& key = app.KeyAt(i);
        key.position = L_ORIGIN + (TARGET_POSITIONS[i] - L_ORIGIN).RotateBy(rotation);
    }
    const fv2 R_ORIGIN = { WIDTH * 0.7f, HEIGHT * 0.5f };
    for (int i : RIGHT_CELLS) {
        auto& key = app.KeyAt(i);
        key.position = R_ORIGIN + (TARGET_POSITIONS[i] - R_ORIGIN).RotateBy(-rotation);
    }

    app.DrawKeys();
}

LimboApp::RotatePerm::RotatePerm(bool reverse, float dura) : Permutation(dura), reverse(reverse) {
    resultingPermutation = P_REVERSE;
}

void LimboApp::RotatePerm::Anim(LimboApp& app, float dt) {
    Permutation::Anim(app, dt);
    const float angle = CubicEase(time) * PI;
    const Rotor2D rotDiff = Radians(-angle);

    app.globalRotation -= angle - currentAngle;
    currentAngle = angle;
    const fv2 ROTATION_ORIGIN = { WIDTH * 0.5f, HEIGHT * 0.5f };

    const float s = Sigmoid(10.0f * (time - 0.5f));
    for (int i = 0; i < 8; ++i) {
        auto& key = app.KeyAt(i);
        if (i % 4 == (reverse ? 0 : 3)) {
            key.position = TARGET_POSITIONS[i].Lerp(TARGET_POSITIONS[(i + (reverse ? 7 : 1)) % 8], s);
        } else {
            key.position = ROTATION_ORIGIN + (TARGET_POSITIONS[i] - ROTATION_ORIGIN).RotateBy(rotDiff);
        }
        // key.z = z;
    }

    app.DrawKeys();
}

LimboApp::DepthSwapPerm::DepthSwapPerm(bool reverse, float dura) : Permutation(dura), reverse(reverse) {
    resultingPermutation = P_SWAP_CELLS;
}

void LimboApp::DepthSwapPerm::Anim(LimboApp& app, float dt) {
    Permutation::Anim(app, dt);
    const float dx = WIDTH * 0.4f * Sigmoid(20 * time - 10),
                dz = Sigmoid(30 * time - 6) - Sigmoid(30 * time - 26);
    for (int i = 0; i < 8; ++i) {
        auto& key = app.KeyAt(i);
        key.z = Z_CENTER * (1 + ((i % 4 < 2) == reverse ? -0.2f : 0.5f) * dz);
        key.position = TARGET_POSITIONS[i] + fv2 { (i % 4 < 2 ? dx : -dx), 0 };
    }

    app.DrawKeys();
}

LimboApp::GlowAnim::GlowAnim(LimboKey& glowingKey, int flashCount, float dura) : Effect(dura), glowingKey(glowingKey), flashCount(flashCount) {}

void LimboApp::GlowAnim::Anim(LimboApp& app, float dt) {
    Effect::Anim(app, dt);
    float s = 0.5f + 0.5f * std::cos(2.0f * flashCount * time * PI);
    glowingKey.colors = app.GetColorShades(0).Lerp(app.GetColorShades(3), 1 - s * s);
    app.DrawKeys();
}

void LimboApp::GlowAnim::Finish(LimboApp& app) {
    Effect::Finish(app);
}

void LimboApp::ReadyAnim::Anim(LimboApp& app, float dt) {
    Effect::Anim(app, dt);

    app.DrawKeys();

    static constexpr float ACC_TIMES[] = { 0.0f, 0.33f, 0.5f, 0.67f, 0.83f, 1.0f };
    static Str TEXTURES[] = { "ready", "3", "2", "1", "go" };
    const int newIdx = (int)(Span { ACC_TIMES }.FindIf([&] (float x) { return x > time; }).UnwrapOr(5)) - 1;
    if (newIdx != texIndex) {
        app.screenShake.Trigger(30.0f);
    }
    texIndex = newIdx;

    const float t = time - ACC_TIMES[texIndex], dur = ACC_TIMES[texIndex + 1] - ACC_TIMES[texIndex];
    const Str tex = TEXTURES[texIndex];

    const fv2& sOff = app.screenShake.offset;
    switch (texIndex) {
        case 0: {
            const float recoilT = std::max((t - (dur - 0.1f)) * 18, -0.05f * 18);
            const float y = CubicEase(t * 18) + (recoilT * recoilT) - 0.81f;
            const fv2 pos = fv2 { WIDTH * 0.5f, HEIGHT * y * 0.5f } + sOff;
            app.DrawTexH(tex, pos, WIDTH * 0.15f);
            break;
        }
        case 1: case 2: case 3: {
            const float k = std::max(std::exp((dur - t) * 24.0f - 1.2f) / std::exp(dur * 12.0f - 0.6f), 1.0f);
            const float rotation = (float[]) { -0.15f, 0.2f, -0.06f } [texIndex - 1];
            app.canvas.transform = { fv2 { WIDTH * 0.5f, HEIGHT * 0.5f } + sOff, WIDTH * 0.2f * k, Radians(rotation) };
            app.DrawTexH(tex, 0, 1, std::min(2.05f - std::exp((dur - t)), 1.0f));
            break;
        }
        case 4: { // GO!
            const float y = CubicEase(t * 18), alpha = 1 - CubicEase((t - (dur - 0.05f)) * 18);
            const fv2 pos = fv2 { WIDTH * 0.5f, HEIGHT * y * 0.5f } + sOff;
            app.DrawTexH(tex, pos, WIDTH * 0.15f * std::exp(1 - alpha), alpha);
            break;
        }
        default:;
    }
}

void LimboApp::ReadyAnim::Finish(LimboApp& app) {
    Effect::Finish(app);
    app.intensify.state = Intensify::USE_ANIM;
}

LimboApp::KeyGizmo::KeyGizmo(LimboApp& app, int i) : Interactable({}), key(app.KeyAt(i)), app(app), keyIndex(i), realZ(key->z) {}

bool LimboApp::KeyGizmo::CaptureEvent(MouseEventType::E e, IO::IO& io) {
    if (e & MouseEventType::CLICK) {
        app->timeline.Skip(*app);
        auto& ending = *app->timeline.CurrentEffect().As<EndAnim>();
        ending.ChooseKey(app, key);
    }
    return Interactable::CaptureEvent(e, io);
}

void LimboApp::KeyGizmo::Update() {
    // if (hovered) app->canvas.DrawRect(hitbox);
    zScale = std::lerp(zScale, hovered ? 0.8f : 1.0f, 0.08f);
    key->z = realZ * zScale;
    key->glowIntensity = std::lerp(key->glowIntensity, hovered ? 0.5f : 0.12f, 0.08f);
    key->scale = std::lerp(key->scale, hovered ? 1.2f : 1.0f, 0.08f);
    const float hitboxScale = KEY_SIZE * Z_CENTER / (realZ * (hovered ? 0.65f : 1.0f));
    hitbox = fRect2D::FromCenter(Project(key->position, key->z), hitboxScale * fv2 { 1.1f, 0.8f });
    capturedEvents = key->z > 0.7f ? 0 : ~0;
    //
    // app->canvas.Fill({ hovered ? 1.0f : 0.0f, capturedEvents == ~0 ? 1.0f : 0.0f, 0.0f, 1.0f });
    // app->canvas.DrawRect(hitbox);
}

LimboApp::ChooseKeyAnim::ChooseKeyAnim(float dura) : Effect(dura) {
    time = 1.7f;
}

void LimboApp::ChooseKeyAnim::Init(LimboApp& app) {
    Effect::Init(app);
    app.intensify.state = Intensify::NO_ANIM;
    app.intensify.aberrationOff = { 6 / WIDTH, -4 / HEIGHT };
    for (int i = 0; i < 8; ++i) {
        keyGizmos[i] = { app, i };
        app.canvas.AddInteractable(keyGizmos[i]);
    }
}

void LimboApp::ChooseKeyAnim::Anim(LimboApp& app, float dt) {
    // dont use this: Permutation::Anim(app, dt);
    time += dt;
    app.globalScale = std::lerp(app.globalScale, 1.0f, 0.05f);

    using namespace Quasi::Math;
    const Radians AXIS_TILT = 85.0_deg;
    const fv3 mainAxis = fv3::FromSpheric(1, Radians(time * 0.3f), AXIS_TILT);
    const Rotor3D tilt = Rotor3D::RotateTo({ 0, 1, 0 }, mainAxis);

    const Rotor2D turn = 45.0_deg;
    Rotor2D curr = Radians(time * 0.6f);

    const fv3 origin = { WIDTH * 0.5f, HEIGHT * 0.5f, Z_CENTER };
    fv3 localX = tilt * fv3 { WIDTH * 0.3f, 0, 0 };
    fv3 localZ = tilt * fv3 { 0, WIDTH * 0.1f, WIDTH * 0.1f };
    localX.z = 0;
    localZ.z /= WIDTH * 0.25f;

    for (int i : { 3, 2, 1, 0, 4, 5, 6, 7 }) {
        auto& key = app.KeyAt(i);
        auto& gizmo = keyGizmos[i];
        auto [x, y, z] = localX * curr.Cos() + localZ * curr.Sin() + origin;
        key.position.LerpToward({ x, y }, 0.05f);
        gizmo.realZ = std::lerp(gizmo.realZ, z, 0.05f);

        gizmo.Update();

        curr += turn;
        const bool h = gizmo.hovered;
        key.colors.LerpTowards({
            app.GetColor(i, h),
            app.GetColor(i, 1) * (h ? 1.2f : 1.0f),
            app.GetColor(i, h ? 0 : 2)
        }, 0.2f);
    }

    const float handY = HEIGHT * (0.46f - 3 * std::exp(0.4f - time) + 0.04f * std::sin(0.6f * time));
    const float a = std::min(time, 1.0f), w = WIDTH * 0.4f * (1 + std::min(time * 0.6f, 1.0f));
    app.DrawTexW("hands",  { WIDTH / 2, handY }, w, a);
    app.DrawTexW("icons",  { WIDTH / 2, HEIGHT * (0.86f + std::exp(3 * (5.0f - time))) }, WIDTH);

    const float chooseY = HEIGHT * (0.5f + 1.8f * std::exp(0.6f - time) + 0.04f * std::sin(0.6f * time + 0.7f));
    app.DrawBackKeys();
    app.DrawTexW("choose", { WIDTH / 2, chooseY }, w * 0.45f, a);
    app.DrawTexH("light", ORIGIN, HEIGHT, 0.2f * std::min(time, 2.0f));
    app.DrawFrontKeys();
}

void LimboApp::ChooseKeyAnim::Finish(LimboApp& app) {
    for (auto& giz : keyGizmos)
        app.canvas.RemoveInteractable(giz);
}

float LimboApp::ChooseKeyAnim::ExtraTime() const {
    return 0;
}

void LimboApp::EndAnim::Init(LimboApp& app) {
    Effect::Init(app);
    app.intensify.state = Intensify::MANUAL;
    app.intensify.aberrationOff = { 6 / WIDTH, -4 / HEIGHT };
    app.intensify.vignetteForeground = true;
    app.intensify.vignetteTint = { 0, 0 };
    app.canvas.NoStroke();
    missileSheet = Texture2D::LoadPNG(RES"exp_compressed.png");
}

void LimboApp::EndAnim::Anim(LimboApp& app, float dt) {
    // Permutation::Anim(app, dt);
    time += dt;
    static constexpr float STAGE_TIMES[] = { 0.0f, 2.0f, 3.0f, 9.4f, 9.4f, 14.0f };
    State newState = (State)Span(STAGE_TIMES).RevFindIf([&] (float x) { return time > x; }).UnwrapOr(0);
    if (state != newState) {
        if (newState - state == 2) { newState = BEFORE_CAPTURE; }
        switch (newState) {
            case SHOW_CORRECT:
                ma_engine_play_sound(&app.audioEngine, correct ? RES"correct.mp3" : RES"incorrect.mp3", nullptr);
                break;
            case BOOM:
                if (!correct)
                    ma_engine_play_sound(&app.audioEngine, RES"vine-boom.mp3", nullptr);
                break;
            case BEFORE_CAPTURE:
                if (!correct)
                    ma_engine_play_sound(&app.audioEngine, RES"mypc.mp3", nullptr);
                app.intensify.EnterExposure(app);
                break;
            case ERROR:
                if (!correct) {
                    FrameBuffer::Screen().Bind(); // draw to screen instead; no post-processing
                    app.intensify.state = Intensify::DISABLED;
                    ma_engine_play_sound(&app.audioEngine, RES"error_sound.mp3", nullptr);
                    app.canvas.AddInteractable(middleErrorMessage);
                }
                break;
            default:;
        }
        state = newState;
    }

    if (state < MISSILE) {
        app.canvas.Fill({ 0, 1 - std::exp(-5.0f * time) });
        app.canvas.NoStroke();
        app.canvas.DrawRect({ 0, { WIDTH, HEIGHT } });
    }

    if (state >= SHOW_CORRECT && state < MISSILE) {
        const float localTime = time - STAGE_TIMES[SHOW_CORRECT];
        app.correctKey->colors.LerpTowards(app.GetColorShades(3) * 1.3, 0.05f);
        app.correctKey->glowIntensity = 0.12f + 0.38f * std::exp(-localTime);

        if (!correct) {
            chosenKey->colors.LerpTowards(app.GetColorShades(0) * 1.1f, 0.05f);
            chosenKey->glowIntensity = 0.12f + 0.38f * std::exp(-localTime);
        }
        app.intensify.innerRadius = std::lerp(1.3f,   0.9f, std::min(1.0f, localTime / 0.4f));
        app.intensify.outerRadius = std::lerp(1.414f, 1.3f, std::min(1.0f, localTime / 0.4f));
        app.intensify.vignetteTint.LerpTowards(app.GetColor(correct ? 3 : 0, 2) * 0.7f, 0.05f);
    }
    if (state < SHOW_CORRECT) {
        for (auto& key : app.keys) {
            key.colors.LerpTowards(chosenKey.RefEquals(key) ?
                Palette { 0.9, 1.0, 0.8 } : Palette { 0, 0, 0 }, 0.2f);
            key.scale = std::lerp(key.scale, 1.0f, 0.05f);
        }
    }

    if (state < MISSILE)
        app.DrawKeys();

    if (state >= SHOW_CORRECT && state < MISSILE) {
        float e = std::exp(3.0f * (2.7f - time));
        app.DrawTexH(correct ? "correct" : "wrong",
            { WIDTH / 2, HEIGHT * (0.83f + 0.3f * e) }, HEIGHT * 0.2f);

        if (!correct && state >= BOOM) {
            float s = std::exp(1.1f * (time - 3.0f));
            app.DrawTexH("wrong", { WIDTH / 2, HEIGHT * (0.83f + 0.3f * e) }, HEIGHT * 0.2f * s, 2 - s);
        }

        if (!correct) {
            const float size = HEIGHT * EaseOutBack(time - 5.0f) * 0.55f, alpha = Clamp(1.05f - 300 * e);
            app.DrawTexH("comp_bg", ORIGIN, size, alpha);
            app.DrawTexH("bsod",    ORIGIN, size, Clamp(1.05f - 7200000.0f * e));
            app.DrawTexH("comp",    ORIGIN, size, alpha);

            app.DrawTexHR("ur_comp", { WIDTH * 0.8f, HEIGHT / 2 }, CubicEase(time - 5.3f) * HEIGHT * 0.55f,
                alpha, (EaseOutBack(time - 5.7f) - 1) * QUART_PI);
        }
    }

    if (!correct && time > 10.0 && state < ERROR) {
        const float animX = Clamp(std::floor(std::log((time - 7.0f) / 3.0f) * 40.0f) / 25.0f);
        app.canvas.DrawSTextureW({ missileSheet, { { animX - 0.04f, 0.0f }, { animX, 1.0f } } }, ORIGIN, WIDTH);
    }

    if (state == ERROR) {
        const float localTime = time - STAGE_TIMES[ERROR];
        app.canvas.Fill(1);
        app.canvas.NoStroke();
        app.canvas.DrawRect({ 0, { WIDTH, HEIGHT } });
        app.DrawTexH("err", ORIGIN, HEIGHT * 0.265f, (localTime > 0.8f || (int)std::floor(localTime * 12) % 2 == 1) ? 1.0f : 0.8f);

        if (app.gdevice.GetIO().Mouse.AnyOnPress()) {
            app.timeline.Skip(app);
            app.timeline.CurrentEffect().As<class Finish>()->SetEnding(!correct);
            app.finished = true;
        }
    }
}

void LimboApp::EndAnim::Finish(LimboApp& app) {
    Effect::Finish(app);
    app.canvas.RemoveInteractable(middleErrorMessage);
}

void LimboApp::EndAnim::ChooseKey(LimboApp& app, LimboKey& key) {
    chosenKey = key;
    correct = app.correctKey.RefEquals(chosenKey);
}

void LimboApp::Finish::Init(LimboApp& app) {
    Effect::Init(app);

#ifndef SAFE_MODE
    System::ChangeWallpaper(L"losers_background.png");
    System::HideIcons();

    if (incorrect) {
        System::Shutdown();
    }
#endif
}

void LimboApp::Finish::SetEnding(bool incorrect) {
    this->incorrect = incorrect;
}

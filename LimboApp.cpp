#include "LimboApp.h"
#include "Resources.h"

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

void LimboApp::Permutation::Finish(LimboApp& app) {
    app.ShuffleKeys(resultingPermutation);
    app.ResetKeyPos();
}

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

LimboApp::LimboApp()
    : gdevice(GraphicsDevice::Initialize({ (int)WIDTH, (int)HEIGHT }, { .fullscreen = true })),
      resources(FETCH_ARCHIVE()) {
    LoadResources();

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
            Boxs::New(Effect        {} ),
            Boxs::New(Finish        { 0 }),
        })
    };

    Render::UseBlendFunc(BlendFactor::ONE, BlendFactor::INVERT_SRC_ALPHA);


    // static constexpr int SAMPLE_RATE = 44100;
    Debug::QInfo$("Total Anim Time: {}", timeline.totalDuration);
    // int SKIP_FRAME_COUNT = (int)((18.54 - animTime) * SAMPLE_RATE);
    // ma_sound_seek_to_pcm_frame(&music, SKIP_FRAME_COUNT);
    postEffect.Init(*this);

#if 1
    PlaySound("LimboMus.mp3");
    ma_sound_set_pitch(&currentSound, 1.0f / INV_SPEED);
    ma_sound_set_volume(&currentSound, 0.2f);
    ma_sound_start(&currentSound);
#endif
}

LimboApp::~LimboApp() {
    ma_engine_uninit(&audioEngine);

    // System::ShowTaskbar();
}

void LimboApp::LoadResources() {
    if (ma_engine_init(nullptr, &audioEngine) != MA_SUCCESS) {
        Debug::QError$("Miniaudio Failed to Load!");
    }

    Image colorSrc = Image::LoadPNGBytes(resources["colorpalette.png"]);

    Vec<Image> sprites; Vec<ImageView> views;
    for (const Str s : {
        "keyhigh.png", "keymain.png", "keyshadow.png", "keyoutline.png",
        "glow.png", "ready.png", "1.png", "2.png", "3.png", "go.png",
        "ominous_hands.png", "spotlight.png", "icons.png", "choose.png",
        "correct.png", "wrong.png", "fake_error.png", "trophy.png",
        "computer.png", "comp_bg.png", "bsod.png", "ur_comp.png", "you_did_it.png" }) {
        views.Push(sprites.Push(Image::LoadPNGBytes(resources[s])));
    }

    texAtlas = TextureAtlas(views,
        { { "high", "main", "shadow", "outline", "glow", "ready", "1", "2", "3", "go",
            "hands", "light", "icons", "choose", "correct", "wrong", "err", "trophy", "comp", "comp_bg",
            "bsod", "ur_comp", "ydi" } },
        true
    );

    for (int i = 0; i < 8; ++i) {
        for (int tone = 0; tone < 3; ++tone) {
            // image is flipped
            colorPalette[i][tone] = (fColor)colorSrc[{ tone, 7 - i }];
        }
        keys[i].colors = colorPalette[0];
    }
}

bool LimboApp::Run() {
    if (finished) return false;

    postEffect.Use();

    gdevice.Begin();
    canvas.BeginFrame();

    // canvas.Fill(fColor::White());
    // canvas.DrawRect({ { WIDTH * 0.05, HEIGHT * 0.15 }, { WIDTH * 0.95, HEIGHT * 0.85 } });
    // canvas.DrawText("Hello, World!", 60, { 560, 740 }, { .rect = { 800, 400 } });
    const auto& io = gdevice.GetIO();
    const float dt = io.Time.DeltaTime();

    if (io.Keyboard.KeyPressed(IO::Key::C)) {
        canvas.DrawRect(fRect2D::FromCenter(
            Project(correctKey->position, correctKey->z),
            fv2 { 200, 150 } / correctKey->z));
    }

    // SetSpinningKeys();
    timeline.Anim(*this, dt);

    // jailbreak safety measure
    if (io.Keyboard.KeyOnPress(IO::Key::F)) {
        return false;
    }

    if (io.Keyboard.KeyOnPress(IO::Key::S)) {
        timeline.Skip(*this);
    }


    canvas.Update(dt);
    postEffect.Anim(*this, dt);

    canvas.Stroke(1);
    canvas.DrawText(Text::Format("FPS = {}", (int)io.Time.Framerate()), 30.0f, { 0, HEIGHT },
        { .alignment = TextAlign::LEFT | TextAlign::VTOP });

    canvas.EndFrame();
    postEffect.Draw();

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
    canvas.transform = Transform2D(screenPos + postEffect.screenShake.offset, 1, Radians(globalRotation));
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

void LimboApp::PlaySound(Str name) {
    if (hasPlayedSound) {
        ma_decoder_uninit(&decoder);
        ma_sound_uninit(&currentSound);
    }
    Bytes mp3Bytes = resources[name];
    ma_decoder_init_memory(mp3Bytes.Data(), mp3Bytes.Length(), nullptr, &decoder);
    ma_sound_init_from_data_source(&audioEngine, (void*)&decoder, 0, nullptr, &currentSound);
    ma_sound_start(&currentSound);
    hasPlayedSound = true;
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
        app.postEffect.screenShake.Trigger(30.0f);
    }
    texIndex = newIdx;

    const float t = time - ACC_TIMES[texIndex], dur = ACC_TIMES[texIndex + 1] - ACC_TIMES[texIndex];
    const Str tex = TEXTURES[texIndex];

    const fv2& sOff = app.postEffect.screenShake.offset;
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
    app.postEffect.state = PostEffect::USE_ANIM;
}

LimboApp::KeyGizmo::KeyGizmo(LimboApp& app, int i) : key(app.KeyAt(i)), app(app), keyIndex(i), realZ(key->z) {}

bool LimboApp::KeyGizmo::CaptureEvent(MouseEventType::E e, IO::IO& io) {
    if (e & MouseEventType::CLICK) {
        const bool correct = key.
        RefEquals(app->correctKey);
        Box ending = correct ? (Box<Effect>)Boxs::New(CorrectEndAnim { key }) : Boxs::New(IncorrectEndAnim { key });
        app->timeline.SkipWith(*app, std::move(ending));
    }
    return Interactable::CaptureEvent(e, io);
}

void LimboApp::KeyGizmo::Update() {
    zScale = std::lerp(zScale, hovered ? 0.8f : 1.0f, 0.08f);
    key->z = realZ * zScale;
    key->glowIntensity = std::lerp(key->glowIntensity, hovered ? 0.5f : 0.12f, 0.08f);
    key->scale = std::lerp(key->scale, hovered ? 1.2f : 1.0f, 0.08f);
    const float hitboxScale = KEY_SIZE * Z_CENTER / (realZ * (hovered ? 0.65f : 1.0f));
    hitbox = fRect2D::FromCenter(Project(key->position, key->z), hitboxScale * fv2 { 1.1f, 0.8f });
    capturedEvents = key->z > 0.7f ? 0 : ~0;
}

LimboApp::ChooseKeyAnim::ChooseKeyAnim(float dura) : Effect(dura) {
    time = 1.7f;
}

void LimboApp::ChooseKeyAnim::Init(LimboApp& app) {
    Effect::Init(app);
    app.postEffect.state = PostEffect::NO_ANIM;
    app.postEffect.aberrationOff = { 6 / WIDTH, -4 / HEIGHT };
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
    app.postEffect.state = PostEffect::MANUAL;
    app.postEffect.aberrationOff = { 6 / WIDTH, -4 / HEIGHT };
    app.postEffect.vignetteForeground = true;
    app.postEffect.vignetteTint = { 0, 0 };
}

void LimboApp::IncorrectEndAnim::Init(LimboApp& app) {
    EndAnim::Init(app);
    missileAnimSheet = Texture2D::LoadPNGBytes(app.resources["exp_compressed.png"]);
    middleErrorMessage = Clickable {
        { { { 704.5, 396.5 }, { 1215.5, 683.5 } } },
        [&] (MouseEventType::E, IO::IO&) {
            app.timeline.Skip(app);
            app.timeline.CurrentEffect().As<class Finish>()->SetEnding(false);
            app.finished = true;
        }
    };
}

void LimboApp::IncorrectEndAnim::Anim(LimboApp& app, float dt) {
    // Permutation::Anim(app, dt);
    time += dt;
    static constexpr float STAGE_TIMES[] = { 0.0f, 2.0f, 3.0f, 7.9f, 9.4f, 9.4f, 14.0f };
    State newState = (State)Span(STAGE_TIMES).RevFindIf([&] (float x) { return time > x; }).UnwrapOr(0);
    if (state != newState) {
        if (newState - state == 2) { newState = BEFORE_CAPTURE; }
        switch (newState) {
            case SHOW_CORRECT:
                app.PlaySound("incorrect.mp3");
                break;
            case BOOM:
                app.PlaySound("vine-boom.mp3");
                break;
            case E_SOUND:
                app.PlaySound("windows_shutdown.mp3");
                break;
            case BEFORE_CAPTURE:
                app.PlaySound("mypc.mp3");
                app.postEffect.CaptureBackground();
                break;
            case ERROR:
                FrameBuffer::Screen().Bind(); // draw to screen instead; no post-processing
                app.postEffect.state = PostEffect::DISABLED;
                app.PlaySound("error_sound.mp3");
                app.canvas.AddInteractable(middleErrorMessage);
                break;
            default:;
        }
        state = newState;
    }

    if (state < MISSILE) {
        app.canvas.Fill({ 0, 1 - std::exp(-5.0f * time) });
        app.canvas.NoStroke();
        app.canvas.DrawRect({ 0, { WIDTH, HEIGHT } });
    } else {
        app.postEffect.DrawBackground(app);
    }

    if (state >= SHOW_CORRECT && state < MISSILE) {
        const float localTime = (time - STAGE_TIMES[SHOW_CORRECT]) / 0.4f, glow = 0.12f + 0.38f * std::exp(-localTime);
        app.correctKey->colors.LerpTowards(app.GetColorShades(3) * 1.3, 0.05f);
        app.correctKey->glowIntensity = glow;
        chosenKey->colors.LerpTowards(app.GetColorShades(0) * 1.1f, 0.05f);
        chosenKey->glowIntensity = glow;

        app.postEffect.innerRadius = std::lerp(1.3f,   0.9f, std::min(1.0f, localTime));
        app.postEffect.outerRadius = std::lerp(1.414f, 1.3f, std::min(1.0f, localTime));
        app.postEffect.vignetteTint.LerpTowards(app.GetColor(0, 2) * 0.7f, 0.05f);
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
        app.DrawTexH("wrong", { WIDTH / 2, HEIGHT * (0.83f + 0.3f * e) }, HEIGHT * 0.2f);

        if (state >= BOOM) {
            float s = std::exp(1.1f * (time - 3.0f));
            app.DrawTexH("wrong", { WIDTH / 2, HEIGHT * (0.83f + 0.3f * e) }, HEIGHT * 0.2f * s, 2 - s);
        }

        const float size = HEIGHT * EaseOutBack(time - 5.0f) * 0.55f, alpha = Clamp(1.05f - 300 * e);
        app.DrawTexH("comp_bg", ORIGIN, size, alpha);
        app.DrawTexH("bsod",    ORIGIN, size, Clamp(1.05f - 7200000.0f * e));
        app.DrawTexH("comp",    ORIGIN, size, alpha);

        app.DrawTexHR("ur_comp", { WIDTH * 0.8f, HEIGHT / 2 }, CubicEase(time - 5.3f) * HEIGHT * 0.55f,
            alpha, (EaseOutBack(time - 5.7f) - 1) * QUART_PI);
    }

    if (time > 10.0 && state < ERROR) {
        const int frame = std::floor(std::log((time - 7.0f) / 3.0f) * 40.0f);
        const float animX = Clamp(frame / 25.0f);
        app.canvas.DrawSTextureW({ missileAnimSheet, { { animX - 0.04f, 0.0f }, { animX, 1.0f } } }, ORIGIN, WIDTH);
        app.canvas.NoStroke();
        app.canvas.Fill({ 1, Clamp((time - 11.0f) * 0.4f) });
        app.canvas.DrawRect({ 0, { WIDTH, HEIGHT } });
    }

    if (state == ERROR) {
        const float localTime = time - STAGE_TIMES[ERROR];
        app.canvas.Fill(1);
        app.canvas.NoStroke();
        app.canvas.DrawRect({ 0, { WIDTH, HEIGHT } });
        app.DrawTexH("err", ORIGIN, HEIGHT * 0.265f, (localTime > 0.8f || (int)std::floor(localTime * 12) % 2 == 1) ? 1.0f : 0.8f);
    }
}

void LimboApp::IncorrectEndAnim::Finish(LimboApp& app) {
    Effect::Finish(app);
    app.canvas.RemoveInteractable(middleErrorMessage);
}

void LimboApp::CorrectEndAnim::Init(LimboApp& app) {
    EndAnim::Init(app);
    partyAnimSheet = Texture2D::LoadPNGBytes(app.resources["party-sheet.png"]);
}

void LimboApp::CorrectEndAnim::Anim(LimboApp& app, float dt) {
    time += dt;
    static constexpr float STAGE_TIMES[] = { 0.0f, 2.0f, 4.0f, 4.0f, 4.9f, 7.0f, 10.0f };
    State newState = (State)Span(STAGE_TIMES).RevFindIf([&] (float x) { return time > x; }).UnwrapOr(0);
    if (state != newState) {
        if (newState - state == 2) { newState = BEFORE_CAPTURE; }
        switch (newState) {
            case SHOW_CORRECT:
                app.PlaySound("correct.mp3");
                break;
            case BEFORE_CAPTURE:
                app.postEffect.CaptureBackground();
                break;
            case PARTY_SOUND:
                app.PlaySound("partyblower.mp3");
                break;
            case TROPHY:
                app.PlaySound("ta-da.mp3");
                break;
            case END:
                app.timeline.Skip(app);
                app.timeline.CurrentEffect().As<class Finish>()->SetEnding(true);
                app.finished = true;
                break;
            default:;
        }
        state = newState;
    }

    if (state < PARTY) {
        app.canvas.Fill({ 0, 1 - std::exp(-5.0f * time) });
        app.canvas.NoStroke();
        app.canvas.DrawRect({ 0, { WIDTH, HEIGHT } });
    } else {
        app.postEffect.DrawBackground(app);
    }

    if (state >= SHOW_CORRECT && state < PARTY) {
        const float localTime = (time - STAGE_TIMES[SHOW_CORRECT]) / 0.4f, glow = 0.12f + 0.38f * std::exp(-localTime);
        app.correctKey->colors.LerpTowards(app.GetColorShades(3) * 1.3, 0.05f);
        app.correctKey->glowIntensity = glow;

        app.postEffect.innerRadius = std::lerp(1.3f,   0.9f, std::min(1.0f, localTime));
        app.postEffect.outerRadius = std::lerp(1.414f, 1.3f, std::min(1.0f, localTime));
        app.postEffect.vignetteTint.LerpTowards(app.GetColor(3, 2) * 0.7f, 0.05f);

        float e = std::exp(3.0f * (2.7f - time));
        app.DrawTexH("correct", { WIDTH / 2, HEIGHT * (0.83f + 0.3f * e) }, HEIGHT * 0.2f);
    } else if (state == BEGIN) {
        for (auto& key : app.keys) {
            key.colors.LerpTowards(chosenKey.RefEquals(key) ?
                Palette { 0.9, 1.0, 0.8 } : Palette { 0, 0, 0 }, 0.2f);
            key.scale = std::lerp(key.scale, 1.0f, 0.05f);
        }
    }

    if (state < PARTY)
        app.DrawKeys();

    if (state == PARTY || state == PARTY_SOUND) {
        const float localTime     = time - STAGE_TIMES[PARTY];
        const int frame           = std::min(std::floor(localTime * 20.0f), 72.0f), x = frame % 8, y = 8 - (frame / 8);
        const fRect2D frameSprite = { { x / 8.0f, y / 9.0f }, { (x + 1) / 8.0f, (y + 1) / 9.0f } };
        const float size          = 512.0f * (Sigmoid((localTime - 0.3f) * 16.0f) - Sigmoid((localTime - 3.3f) * 16.0f));
        const Rotor2D rotation    = 90.0_deg * (1 - Sigmoid((localTime - 0.6f) * 8.0f) - Sigmoid((localTime - 3.0f) * 8.0f));
        const fv2 position        = rotation.InvRotate({ WIDTH * 0.5f, HEIGHT * 0.4f }) + fv2 { 0.148f, 0.273f } * size;

        app.canvas.transform = { 0, 1, rotation };
        app.canvas.DrawSTextureW({ partyAnimSheet, frameSprite }, position, size);
        app.canvas.transform.Reset();

        if (particles.Length() < 270) {
            AddParticle(app, { WIDTH * -0.3f, HEIGHT * 1.0f }, -PI / 10.0f);
            AddParticle(app, { WIDTH *  1.3f, HEIGHT * 1.0f }, PI * 11.0f / 10.0f);
            AddParticle(app, { WIDTH *  0.5f, HEIGHT * 1.7f }, -HALF_PI);
        }
    }

    if (state == TROPHY) {
        app.DrawTexW("trophy", { WIDTH * 0.5f, HEIGHT * 0.4f }, WIDTH * 0.4f * EaseOutBack(time - STAGE_TIMES[TROPHY]));
        app.DrawTexH("ydi", { WIDTH * 0.5f, HEIGHT * 0.12f }, HEIGHT * 0.3f * EaseOutBack(time - STAGE_TIMES[TROPHY] - 0.4f));
    }

    if (state >= PARTY) {
        UpdateParticles(dt);
        DrawParticles(app);
    }
}

void LimboApp::CorrectEndAnim::Finish(LimboApp& app) {
    EndAnim::Finish(app);
}

void LimboApp::CorrectEndAnim::AddParticle(LimboApp& app, const fv2& center, float angle) {
    auto& rand = app.gdevice.GetRand();

    Particle p;
    p.position = center;
    p.z = rand.Get(0.6f, 1.6f);
    p.velocity    = fv2::FromPolar(WIDTH * rand.Get(0.2f, 0.4f), Radians(angle) + Degrees(rand.Get(-30.0f, 30.0f)));
    p.angVelocity = fv3::RandomInUnit(rand) * PI;
    p.angle       = Rotor3D::Random(rand).AsQuat();
    p.shape       = rand.Get(0, 8);
    particles.Push(p);
}

void LimboApp::CorrectEndAnim::UpdateParticles(float dt) {
    static constexpr float GRAVITY = HEIGHT * 0.25f, DRAG = 0.001f;
    for (auto& p : particles) {
        // cull test
        if (p.position.y < HEIGHT * 0.5f * (0.95f - p.z)) {
            p.shape |= 32;
            continue;
        }
        p.position += p.velocity * dt;
        p.velocity += (fv2 { 0, -GRAVITY } - DRAG * p.velocity.Len() * p.velocity) * dt;
        p.angle    += (0.5f * dt) * Quaternion(0, p.angVelocity) * p.angle;
    }
}

void LimboApp::CorrectEndAnim::DrawParticles(LimboApp& app) {
    static const fColor COLORS[4] = {
        0x2cdb5b_rgbf, 0xf03629_rgbf, 0xedc218_rgbf, 0x1878ed_rgbf
    };
    app.canvas.Stroke(1);
    int numRendered = 0;
    for (auto& p : particles) {
        if (p.shape & 32) continue;
        // const fv2 center = Project(p.position, p.z);
        // app.canvas.DrawPoint(center);
        app.canvas.NoStroke();
        app.canvas.Fill(COLORS[p.shape & 3]);
        const Rotor3D rotor = Rotor3D::FromQuat(p.angle);
        const fv2 center = Project(p.position, p.z);

        if (p.shape & 4) {
            const float s = 12.0f / p.z;
            const fv2 a1 = rotor.Rotate(fv3 { s, 0, 0 }).As2D(),
                      a2 = rotor.Rotate(fv3 { 0, s, 0 }).As2D();
            app.canvas.DrawEllipse(center, a1, a2);
        } else {
            const float s = 10.0f / p.z;
            const fv2 c1 = rotor.Rotate(fv3 { s * 2.0f,  s, 0 }).As2D(),
                      c2 = rotor.Rotate(fv3 { s * 2.0f, -s, 0 }).As2D();
            app.canvas.DrawQuad(center + c1, center + c2, center - c1, center - c2);
        }
        ++numRendered;
    }
    if (numRendered == 0) {
        particles.Clear();
    }
}

void LimboApp::Finish::Init(LimboApp& app) {
    Effect::Init(app);

#ifndef SAFE_MODE
    System::ChangeWallpaper(L"losers_background.png");
    System::HideIcons();

    if (!correct) {
        System::Shutdown();
    }
#endif
}

void LimboApp::Finish::SetEnding(bool correct) {
    this->correct = correct;
}

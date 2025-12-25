#include "LimboApp.h"
#include "Utils/Algorithm.h"

// A B C D
// E F G H
#define P_SWAP_ROWS  "EFGHABCD"
#define P_SWAP_COLS  "BADCFEHG"
#define P_SHIFT_CW   "EABCFGHD"
#define P_SHIFT_CCW  "BCDHAEFG"
#define P_EASY_SHIFT "ECBHAGFD"
#define P_CYCLE_CELL_CW  { 1, 5, 6, 2, 0, 4, 7, 3 }
#define P_CYCLE_CELL_CCW { 4, 0, 3, 7, 5, 1, 2, 6 }

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
            FACTORY(Box<ShiftPerm>::Build(0)),
            FACTORY(Box<ShiftPerm>::Build(1)),
            FACTORY(Box<ShiftPerm>::Build(2)),
            FACTORY(Box<ShiftPerm>::Build(3)),
            FACTORY(Box<ShiftPerm>::Build(4)),
            FACTORY(Box<ShiftPerm>::Build(5)),
            FACTORY(Box<ShiftPerm>::Build(6)),
            FACTORY(Box<ShiftPerm>::Build(7)),
            FACTORY(Box<ShiftPerm>::Build(8)),
            FACTORY(Box<ShiftPerm>::Build(9)),
            FACTORY(Box<ShiftPerm>::Build(10)),
            FACTORY(Box<ShiftPerm>::Build(11)),
            FACTORY(Box<ShufflePerm>::Build(P_EASY_SHIFT)),
            FACTORY(Box<ShiftPerm>::Build(0)),
            FACTORY(Box<RotatePerm>::Build(false, true)),
            FACTORY(Box<ShufflePerm>::Build(P_SWAP_COLS)),
            FACTORY(Box<RotatePerm>::Build(true, true)),
            FACTORY(Box<ShufflePerm>::Build(P_SHIFT_CCW)),
            FACTORY(Box<RotatePerm>::Build(false, false)),
            FACTORY(Box<ShufflePerm>::Build(P_SWAP_ROWS)),
            FACTORY(Box<RotatePerm>::Build(true, false)),
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
        ResetKeyPos();
        currentPerm = permutations[frame]();
        currentPerm->Init(*this);
        ++frame %= permutations.Length();
    }

    canvas.EndFrame();
    gdevice.End();
    return true;
}

void LimboApp::DrawKey(Math::fv2 pos, float scale, int colorIndex) {
    canvas.DrawTextureW(texKeyMain,    pos, scale, true, { .tint = colorPalette[colorIndex][0] });
    canvas.DrawTextureW(texKeyHigh,    pos, scale, true, { .tint = colorPalette[colorIndex][1] });
    canvas.DrawTextureW(texKeyShadow,  pos, scale, true, { .tint = colorPalette[colorIndex][2] });
    canvas.DrawTextureW(texKeyOutline, pos, scale);
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
        DrawKey(i, 200, i);
    }

    // canvas.Stroke(Math::fColor::White());
    // canvas.DrawText("Limbo", 216, { WIDTH * 0.3, HEIGHT * 0.7 }, { .rect = { WIDTH * 0.4, HEIGHT * 0.4 } });

    for (int i = 0; i < 8; i++) { // front keys
        if (keys[i].z >= Z_CENTER) continue;
        DrawKey(i, 200, i);
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
        keys[j].position.LerpToward(TARGET_POSITIONS[i], 0.15f);
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

void LimboApp::ShufflePerm::Init(LimboApp& app) {
    Permutation::Init(app);
    Algorithm::ApplyPermutation(app.keyPermutation.AsSpan(), Spans::Vals(indices));
}

void LimboApp::ShufflePerm::Anim(LimboApp& app, float dt) {
    for (int i = 0; i < 8; ++i) {
        const int j = app.keyPermutation[i];
        app.keys[j].position.LerpToward(TARGET_POSITIONS[i], 0.15f);
    }
}

void LimboApp::RotatePerm::Anim(LimboApp& app, float dt) {
    const float lerpAngle = std::min((Math::PI - currentRotation) * 0.1f, Math::PI * dt * 2.0f);
    currentRotation += lerpAngle;
    const Math::Rotor2D rotDiff = Math::Radians(-f32s::Signed(clockwise, lerpAngle));
    if (cellwise) {
        const Math::fv2 LEFT_ORIGIN = { WIDTH * 0.3f, HEIGHT * 0.5f };
        for (int i : { 0, 1, 4, 5 }) {
            auto& key = app.keys[app.keyPermutation[i]];
            key.position = LEFT_ORIGIN + (key.position - LEFT_ORIGIN).RotateBy(rotDiff);
        }
        const Math::fv2 RIGHT_ORIGIN = { WIDTH * 0.7f, HEIGHT * 0.5f };
        for (int i : { 2, 3, 6, 7 }) {
            auto& key = app.keys[app.keyPermutation[i]];
            key.position = RIGHT_ORIGIN + (key.position - RIGHT_ORIGIN).RotateBy(-rotDiff);
        }
    } else {
        const float t = currentRotation / Math::PI;
        const float z = 1 + 2 * t * (1 - t);
        for (auto& key : app.keys) {
            key.position = ORIGIN + (key.position - ORIGIN).RotateBy(rotDiff);
            key.z = z;
        }
    }
}

void LimboApp::RotatePerm::Finish(LimboApp& app) {
    Permutation::Finish(app);
    if (cellwise) {
        Algorithm::ApplyPermutation(
            app.keyPermutation.AsSpan(),
            clockwise ? Spans::Vals(P_CYCLE_CELL_CW) : Spans::Vals(P_CYCLE_CELL_CCW)
        );
    } else {
        app.keyPermutation.Reverse();
    }
}

void LimboApp::ShiftPerm::Init(LimboApp& app) {
    Permutation::Init(app);
    const bool horizontal = 0b110000110000 & (1 << direction), positive = direction < 6;
    stride = i32s::Signed(positive, horizontal ? -1 : 4);
    // A B C D D H H G F E E A
    warpSrcIndex  = (const int[13]) { 0, 1, 2, 3, 3, 7, 7, 6, 5, 4, 4, 0 } [direction];
    // E F G H A E D C B A H D
    warpDestIndex = (const int[13]) { 4, 5, 6, 7, 0, 4, 3, 2, 1, 0, 7, 3 } [direction];

    warpKey = app.keys[app.keyPermutation[warpSrcIndex]];
    // phantomKey = TARGET_POSITIONS[warpSrcIndex];
    offset = horizontal ? Math::fv2 { f32s::Signed(positive, WIDTH), 0 } :
                          Math::fv2 { 0, f32s::Signed(positive, HEIGHT) };
    // warpKey->position -= off;
    // phantomDest = phantomKey + off;
}

void LimboApp::ShiftPerm::Anim(LimboApp& app, float dt) {
    time += dt;
    static constexpr float INIT_Y = 0.00247875217667f, S = 15.9784934032;
    app.DrawKey(TARGET_POSITIONS[warpSrcIndex] + offset * (S * (std::exp(11 * time - 6) - INIT_Y)), 200.0f, 0);

    for (int i = warpSrcIndex + stride; i != warpDestIndex + stride; i += stride) {
        auto& key = app.keys[app.keyPermutation[i]];
        key.position.LerpToward(TARGET_POSITIONS[i - stride], 0.15f);
    }
    warpKey->position = TARGET_POSITIONS[warpDestIndex] - offset * (S * std::max((std::exp(-2.4f - 11 * time) - INIT_Y), 0.0f));
}

void LimboApp::ShiftPerm::Finish(LimboApp& app) {
    Permutation::Finish(app);
    if (std::abs(stride) == 1) { // horizontal shift
        app.keyPermutation.SubspanMut(warpSrcIndex - (warpSrcIndex % 4), 4).RotateSigned(-stride);
    } else {
        std::swap(app.keyPermutation[warpSrcIndex], app.keyPermutation[warpDestIndex]);
    }
}

#pragma once
#include "Timeline.h"
#include "PostEffect.h"
#include "GUI/Canvas.h"
#include "Quasi/src/Graphics/GraphicsDevice.h"
#include "miniaudio/miniaudio.h"

using namespace Quasi;

struct LimboKey {
    Math::fv2 position;
    float scale = 1, z = 1, glowIntensity = 0.12f;
    Math::fColor color[3];
};

struct ScreenShake {
    Math::fv2 offset;
    float amplitude = 0.0f;
    void Trigger(float amp);
    void Update(float dt);
};

class LimboApp {
    class Permutation : public Effect {
    protected:
        Array<int, 8> resultingPermutation = { 0, 1, 2, 3, 4, 5, 6, 7, };
    public:
        explicit Permutation(float dura) : Effect(dura) {}
        ~Permutation() override = default;
        void Finish(LimboApp& app) override;
    };

    class Intensify : public Effect {
    public:
        Graphics::PostEffect postEffect;
        float innerRadius = 0, outerRadius = 0;
        Math::iv2 aberrationOff = { 3, -2 };
        Math::fColor vignetteTint = { 0, 0 };
        bool enabled = false, manual = false, vignetteForeground = false;

        Intensify() = default;
        Intensify(Graphics::GraphicsDevice& gdevice);

        void Anim(LimboApp& app, float dt) override;
        void Reset(LimboApp& app);
        void Use();
        void Draw();
    };

    static constexpr float WIDTH = 1920, HEIGHT = 1080, Z_CENTER = 1.0f, KEY_SIZE = WIDTH * 0.1;
    static const Math::fv2 ORIGIN;

    Graphics::GraphicsDevice gdevice;
    Graphics::Canvas canvas { gdevice };
    ma_engine audioEngine;
    ma_sound music;

    ScreenShake screenShake;
    Intensify intensify;

    // Graphics::Bloom bloom;

    LimboKey keys[8];
    OptRef<LimboKey> correctKey = nullptr;
    Array<int, 8> keyPermutation = { 0, 1, 2, 3, 4, 5, 6, 7 };
    f32 globalRotation = 0.0f, globalScale = 1.0f;
    bool showHitboxes = false;

    Timeline timeline;
    static const Math::fv2 TARGET_POSITIONS[8];

    Graphics::TextureAtlas texAtlas;
    Math::fColor colorPalette[8][3];
public:
    LimboApp();
    ~LimboApp();

    bool Run();

    static Math::fv2 Project(Math::fv2 position, float z);

    void DrawKey(int index);
    void DrawFrontKeys();
    void DrawBackKeys();
    void DrawKeys();
    void DrawTexW(Str name, const Math::fv2& pos, float w, float alpha = 1);
    void DrawTexH(Str name, const Math::fv2& pos, float h, float alpha = 1);

    const Math::fColor& GetColor(int index, int shade) const;
    const CArray<Math::fColor, 3>& GetColorShades(int index) const;
    const LimboKey& KeyAt(int index) const;
    LimboKey& KeyAt(int index);

    void ResetKeyPos();
    void LerpKeyPos();
    // indices is the array of 'what each key is replaced with'
    void ShuffleKeys(Span<int> indices);

    class ShufflePerm : public Permutation {
    public:
        explicit ShufflePerm(Str permutation, float dura);
        ~ShufflePerm() override = default;
        void Anim(LimboApp& app, float dt) override;
    };

    class CyclicPerm : public Permutation {
        bool ccw = true;
    public:
        explicit CyclicPerm(bool ccw, float dura);
        ~CyclicPerm() override = default;
        void Anim(LimboApp& app, float dt) override;
    };

    class RotatePerm : public Permutation {
        bool reverse = false;
        float currentAngle = 0.0f;
    public:
        explicit RotatePerm(bool reverse, float dura);
        ~RotatePerm() override = default;
        void Anim(LimboApp& app, float dt) override;
    };

    class DepthSwapPerm : public Permutation {
        bool reverse;
    public:
        explicit DepthSwapPerm(bool reverse, float dura);
        ~DepthSwapPerm() override = default;
        void Anim(LimboApp& app, float dt) override;
    };

    class GlowAnim : public Effect {
        LimboKey& glowingKey;
        int flashCount = 3;
    public:
        explicit GlowAnim(LimboKey& glowingKey, int flashCount, float dura);
        ~GlowAnim() override = default;
        void Anim(LimboApp& app, float dt) override;
        void Finish(LimboApp& app) override;
    };

    class ReadyAnim : public Effect {
        int texIndex = 0;
    public:
        explicit ReadyAnim(float dura) : Effect(dura) {}
        ~ReadyAnim() override = default;
        void Anim(LimboApp& app, float dt) override;
        void Finish(LimboApp& app) override;
    };

    struct KeyGizmo : Interactable {
        OptRef<LimboKey> key = nullptr;
        OptRef<LimboApp> app;
        int keyIndex = 0;
        float realZ = 1.0f, zScale = 1.0f;
        KeyGizmo() : Interactable({}) {}
        KeyGizmo(LimboApp& app, int i);
        ~KeyGizmo() override = default;

        bool CaptureEvent(MouseEventType::E e, IO::IO& io) override;
        void Update();
    };

    class ChooseKeyAnim : public Effect {
        Array<KeyGizmo, 8> keyGizmos;
    public:
        explicit ChooseKeyAnim(float dura);
        ~ChooseKeyAnim() override = default;
        void Init(LimboApp& app) override;
        void Anim(LimboApp& app, float dt) override;
        void Finish(LimboApp& app) override;
        float ExtraTime() const override;
        bool Done() const override { return false; }
    };

    class EndAnim : public Effect {
        OptRef<LimboKey> chosenKey = nullptr;
    public:
        explicit EndAnim(float dura) : Effect(dura) {}

        void Init(LimboApp& app) override;
        void Anim(LimboApp& app, float dt) override;

        void ChooseKey(LimboKey& key);
        bool Done() const override { return false; }
    };
};
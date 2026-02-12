#pragma once
#include "Timeline.h"
#include "PostEffect.h"
#include "GUI/Canvas.h"
#include "Quasi/src/Graphics/GraphicsDevice.h"
#include "miniaudio/miniaudio.h"


struct Palette : Array<fColor, 3> {
    Palette() = default;
    Palette(const fColor& a, const fColor& b, const fColor& c) : Array(a, b, c) {};

    Palette operator*(float x) const;
    void LerpTowards(const Palette& p, float t);
    Palette Lerp(const Palette& p, float t) const;
};

struct LimboKey {
    fv2 position;
    float scale = 1, z = 1, glowIntensity = 0.12f;
    Palette colors;
};

#define RES "../res/"
static constexpr float WIDTH = 1920, HEIGHT = 1080, Z_CENTER = 1.0f, KEY_SIZE = WIDTH * 0.1;
inline static const fv2 ORIGIN = { WIDTH / 2, HEIGHT / 2 };

class LimboApp {
    class Permutation : public Effect {
    protected:
        Array<int, 8> resultingPermutation = { 0, 1, 2, 3, 4, 5, 6, 7, };
    public:
        explicit Permutation(float dura) : Effect(dura) {}
        ~Permutation() override = default;
        void Finish(LimboApp& app) override;
    };

    GraphicsDevice gdevice;
    Canvas canvas { gdevice };
    ma_engine audioEngine;
    ma_sound music;

    PostEffect postEffect;

    // Graphics::Bloom bloom;

    LimboKey keys[8];
    OptRef<LimboKey> correctKey = nullptr;
    Array<int, 8> keyPermutation = { 0, 1, 2, 3, 4, 5, 6, 7 };
    f32 globalRotation = 0.0f, globalScale = 1.0f;

    Timeline timeline;
    bool finished = false;
    static const fv2 TARGET_POSITIONS[8];

    TextureAtlas texAtlas;
    Palette colorPalette[8];

    friend PostEffect;
public:
    LimboApp();
    ~LimboApp();

    bool Run();

    static fv2 Project(fv2 position, float z);

    void DrawKey(int index);
    void DrawFrontKeys();
    void DrawBackKeys();
    void DrawKeys();
    void DrawTexW(Str name, const fv2& pos, float w, float alpha = 1);
    void DrawTexH(Str name, const fv2& pos, float h, float alpha = 1);
    void DrawTexHR(Str name, const fv2& pos, float h, float alpha = 1, float angle = 0);

    const fColor& GetColor(int index, int shade) const;
    const Palette& GetColorShades(int index) const;
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
    protected:
        OptRef<LimboKey> chosenKey = nullptr;
    public:
        explicit EndAnim(LimboKey& key) : Effect(1.0f), chosenKey(key) {}
        void Init(LimboApp& app) override;
        bool Done() const override { return false; }
    };

    class IncorrectEndAnim : public EndAnim {
        enum State { BEGIN, SHOW_CORRECT, BOOM, BEFORE_CAPTURE, MISSILE, ERROR, END } state = BEGIN;
        Texture2D missileAnimSheet;
        Interactable middleErrorMessage = { { { 704.5, 396.5 }, { 1215.5, 683.5 } } };
    public:
        using EndAnim::EndAnim;
        void Init(LimboApp& app) override;
        void Anim(LimboApp& app, float dt) override;
        void Finish(LimboApp& app) override;
    };

    class CorrectEndAnim : public EndAnim {
        enum State { BEGIN, SHOW_CORRECT, PARTY } state = BEGIN;
        Texture2D partyAnimSheet;

        struct Particle {
            fv2 position; fv2 velocity; float z;
            Quaternion angle; fv3 angVelocity;
            int shape;
        };
        Vec<Particle> particles;
    public:
        using EndAnim::EndAnim;
        void Init(LimboApp& app) override;
        void Anim(LimboApp& app, float dt) override;
        void Finish(LimboApp& app) override;

        void AddParticle(LimboApp& app);
        void UpdateParticles(float dt);
        void DrawParticles(LimboApp& app);
    };

    class Finish : public Effect {
        bool incorrect = true;
    public:
        explicit Finish(float dura) : Effect(dura) {}
        void Init(LimboApp& app) override;
        void SetEnding(bool incorrect);
    };
};
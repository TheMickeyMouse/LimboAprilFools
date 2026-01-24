#pragma once
#include "Timeline.h"
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

    static constexpr float WIDTH = 1920, HEIGHT = 1080, Z_CENTER = 1.0f, KEY_SIZE = WIDTH * 0.1;
    static const Math::fv2 ORIGIN;

    Graphics::GraphicsDevice gdevice;
    Graphics::Canvas canvas { gdevice };
    ma_engine audioEngine;
    ma_sound music;

    Graphics::RenderObject<Graphics::Vertex2D> vignette;
    ScreenShake screenShake;
    float effectTimer = 0.0f;

    LimboKey keys[8];
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
    void DrawKeys();

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

    // not a permutation but used for animations
    class GlowAnim : public Permutation {
        LimboKey& glowingKey;
        int flashCount = 3;
    public:
        explicit GlowAnim(LimboKey& glowingKey, int flashCount, float dura);
        ~GlowAnim() override = default;
        void Anim(LimboApp& app, float dt) override;
        void Finish(LimboApp& app) override;
    };

    class ReadyAnim : public Permutation {
        int texIndex = 0;
    public:
        explicit ReadyAnim(float dura) : Permutation(dura) {}
        ~ReadyAnim() override = default;
        void LateAnim(LimboApp& app, float dt) override;
    };

    struct KeyGizmo : Interactable {
        OptRef<LimboKey> key = nullptr;
        int keyIndex = 0;
        float realZ = 1.0f, zScale = 1.0f;
        KeyGizmo() : Interactable({}) {}
        KeyGizmo(LimboKey& key, int index);
        ~KeyGizmo() override = default;

        bool CaptureEvent(MouseEventType::E e, IO::IO& io) override;
        void Update();
    };

    class EndAnim : public Permutation {
        Array<KeyGizmo, 8> keyGizmos;
    public:
        explicit EndAnim(float dura);
        ~EndAnim() override = default;
        void Init(LimboApp& app) override;
        void Anim(LimboApp& app, float dt) override;
        bool Done() const override { return false; }
    };
};
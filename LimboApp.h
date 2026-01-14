#pragma once
#include "GUI/Canvas.h"
#include "Quasi/src/Graphics/GraphicsDevice.h"
#include "miniaudio/miniaudio.h"

using namespace Quasi;

struct LimboKey {
    Math::fv2 position;
    float z;
    bool visible = true;
};

class LimboApp {
    class Permutation {
    protected:
        float time = 0.0f;
        Array<int, 8> resultingPermutation = { 0, 1, 2, 3, 4, 5, 6, 7, };
    public:
        float duration = 1.0f;

        explicit Permutation(float dura) : time(-1.0f / (60 * dura)), duration(dura) {}
        virtual ~Permutation() = default;
        virtual void Anim(LimboApp& app, float dt);
        virtual void LateAnim(LimboApp& app, float dt) {}
        virtual void Finish(LimboApp& app);

        bool Done() const { return time >= 1.0f; }
        float ExtraTime() const { return (time - 1.0f) * duration; }
        void AddTime(float dt) { time += dt / duration; }
    };

    static constexpr float WIDTH = 1920, HEIGHT = 1080, Z_CENTER = 1.0f, KEY_SIZE = WIDTH * 0.1;
    static const Math::fv2 ORIGIN;

    Graphics::GraphicsDevice gdevice;
    Graphics::Canvas canvas { gdevice };
    ma_engine audioEngine;
    ma_sound music;

    LimboKey keys[8];
    Array<int, 8> keyPermutation = { 0, 1, 2, 3, 4, 5, 6, 7 };
    usize frame = 0;
    f32 globalRotation = 0.0f;
    bool showColors = false;

    // this is a list of factories
    Vec<Box<Permutation>> permutations;
    OptRef<Permutation> currentPerm = nullptr;
    static const Math::fv2 TARGET_POSITIONS[8];

    Graphics::TextureAtlas texAtlas;
    Math::fColor colorPalette[8][3];
public:
    LimboApp();
    ~LimboApp();

    bool Run();

    static Math::fv2 Project(Math::fv2 position, float z);

    void DrawKey(Math::fv2 pos, float scale, const Math::fColor palette[3]);
    void DrawKey(Math::fv2 pos, float scale, int colorIndex);
    void DrawKey(int index, float scale = KEY_SIZE, int overrideColorIndex = -1);
    void DrawKeys();

    void ResetKeyPos();
    void LerpKeyPos();
    // indices is the array of 'what each key is replaced with'
    void ShuffleKeys(Span<int> indices);
    void SetSpinningKeys();

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
        int glowingKey = 0, flashCount = 3;
    public:
        explicit GlowAnim(int glowingKey, int flashCount, float dura);
        ~GlowAnim() override = default;
        void Anim(LimboApp& app, float dt) override;
        void Finish(LimboApp& app) override;
    };

    class ReadyAnim : public Permutation {
    public:
        explicit ReadyAnim(float dura) : Permutation(dura) {}
        ~ReadyAnim() override = default;
        void LateAnim(LimboApp& app, float dt) override;
    };
};
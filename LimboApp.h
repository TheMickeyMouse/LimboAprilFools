#pragma once
#include "GUI/Canvas.h"
#include "Quasi/src/Graphics/GraphicsDevice.h"
#include "miniaudio/miniaudio.h"

using namespace Quasi;

struct LimboKey {
    Math::fv2 position;
    float z;
};

class LimboApp {
    class Permutation {
    protected:
        float time = 0.0f;
    public:
        float duration = 1.0f;

        explicit Permutation(float dura) : time(-1.0f / (60 * dura)), duration(dura) {}
        virtual ~Permutation() = default;
        virtual void Init(LimboApp& app) {}
        virtual void Anim(LimboApp& app, float dt);
        virtual void Finish(LimboApp& app) {}

        bool Done() const { return time > 1.0f; }
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

    // this is a list of factories
    Vec<Box<Permutation>> permutations;
    OptRef<Permutation> currentPerm = nullptr;
    static const Math::fv2 TARGET_POSITIONS[8];

    Graphics::Texture2D texKeyMain, texKeyHigh, texKeyShadow, texKeyOutline;
    Math::fColor colorPalette[8][3];
public:
    LimboApp();
    ~LimboApp();

    bool Run();

    static Math::fv2 Project(Math::fv2 position, float z);

    void DrawKey(Math::fv2 pos, float scale, int colorIndex);
    void DrawKey(int index, float scale = KEY_SIZE, int overrideColorIndex = -1);
    void DrawKeys();

    void ResetKeyPos();
    void LerpKeyPos();
    void SetSpinningKeys();

    class ShufflePerm : public Permutation {
        int indices[8] {};
    public:
        explicit ShufflePerm(Str permutation, float dura);
        ~ShufflePerm() override = default;
        void Anim(LimboApp& app, float dt) override;
        void Finish(LimboApp& app) override;
    };

    class CyclicPerm : public Permutation {
        bool clockwise = true;
    public:
        explicit CyclicPerm(bool clockwise, float dura) : Permutation(dura), clockwise(clockwise) {}
        ~CyclicPerm() override = default;
        void Anim(LimboApp& app, float dt) override;
        void Finish(LimboApp& app) override;
    };

    class RotatePerm : public Permutation {
        bool reverse = false;
        float currentAngle = 0.0f;
    public:
        explicit RotatePerm(bool reverse, float dura) : Permutation(dura), reverse(reverse) {}
        ~RotatePerm() override = default;
        void Anim(LimboApp& app, float dt) override;
        void Finish(LimboApp& app) override;
    };

    class DepthSwapPerm : public Permutation {
        bool reverse;
    public:
        explicit DepthSwapPerm(bool reverse, float dura) : Permutation(dura), reverse(reverse) {}
        ~DepthSwapPerm() override = default;
        void Anim(LimboApp& app, float dt) override;
        void Finish(LimboApp& app) override;
    };
};
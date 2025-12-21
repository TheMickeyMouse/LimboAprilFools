#pragma once
#include "GUI/Canvas.h"
#include "Quasi/src/Graphics/GraphicsDevice.h"

using namespace Quasi;

struct LimboKey {
    Math::fv2 position;
    float z;
    // Math::Rotor3D rotation;
};

class LimboApp {
    class Permutation {
    public:
        virtual ~Permutation() = default;
        virtual void Anim(LimboApp& app, float dt) = 0;
        virtual void Finish(LimboApp& app) {}
    };

    static constexpr float WIDTH = 1920, HEIGHT = 1080, Z_CENTER = 1.0f;
    static const Math::fv2 ORIGIN;

    Graphics::GraphicsDevice gdevice;
    Graphics::Canvas canvas { gdevice };

    LimboKey keys[8];
    Array<int, 8> keyPermutation = { 0, 1, 2, 3, 4, 5, 6, 7 };
    usize frame = 0;

    // this is a list of factories
    Vec<Box<Permutation>(*)()> permutations;
    Box<Permutation> currentPerm = nullptr;
    static const Math::fv2 TARGET_POSITIONS[8];

    Graphics::Texture2D texKeyMain, texKeyHigh, texKeyShadow, texKeyOutline;
    Math::fColor colorPalette[8][3];
public:
    LimboApp();

    bool Run();

    static Math::fv2 Project(Math::fv2 position, float z);

    void DrawKey(int index, float scale = 200, int overrideColorIndex = -1);
    void DrawKeys();

    void ResetKeyPos();
    void LerpKeyPos();
    void SetSpinningKeys();
    void PerformPermutation(Str newPerm);

    class ShufflePerm : public Permutation {
        int indices[8] {};
    public:
        explicit ShufflePerm(Str permutation);
        ~ShufflePerm() override = default;
        void Anim(LimboApp& app, float dt) override;
        void Finish(LimboApp& app) override;
    };

    class RotatePerm : public Permutation {
        float currentRotation = 0.0f;
    public:
        explicit RotatePerm() = default;
        ~RotatePerm() override = default;
        void Anim(LimboApp& app, float dt) override;
        void Finish(LimboApp& app) override;
    };
};
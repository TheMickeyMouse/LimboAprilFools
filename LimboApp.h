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
        virtual void Init(LimboApp& app) {}
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

    void DrawKey(Math::fv2 pos, float scale, int colorIndex);
    void DrawKey(int index, float scale = 200, int overrideColorIndex = -1);
    void DrawKeys();

    void ResetKeyPos();
    void LerpKeyPos();
    void SetSpinningKeys();

    class ShufflePerm : public Permutation {
        int indices[8] {};
    public:
        explicit ShufflePerm(Str permutation);
        ~ShufflePerm() override = default;
        void Init(LimboApp& app) override;
        void Anim(LimboApp& app, float dt) override;
    };

    class RotatePerm : public Permutation {
        bool clockwise = true, cellwise = false;
        float currentRotation = 0.0f;
    public:
        explicit RotatePerm(bool clockwise, bool cellwise) : clockwise(clockwise), cellwise(cellwise) {}
        ~RotatePerm() override = default;
        void Anim(LimboApp& app, float dt) override;
        void Finish(LimboApp& app) override;
    };

    class ShiftPerm : public Permutation {
        // directions are as follows:
        //      0 1 2 3
        //      ^ ^ ^ ^
        // 11 < A B C D > 4
        // 10 < E F G H > 5
        //      v v v v
        //      9 8 7 6
        int direction = 0;
        int stride = 0;
        int warpSrcIndex = 0;
        int warpDestIndex = 0;
        float time = 0.0f;
        OptRef<LimboKey> warpKey = nullptr;
        Math::fv2 offset;
    public:
        explicit ShiftPerm(int dir) : direction(dir) {}
        ~ShiftPerm() override = default;
        void Init(LimboApp& app) override;
        void Anim(LimboApp& app, float dt) override;
        void Finish(LimboApp& app) override;
    };
};
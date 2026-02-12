#pragma once
#include "GLs/FrameBuffer.h"
#include "GLs/RenderBuffer.h"
#include "GLs/Shader.h"
#include "GLs/Texture.h"

namespace Quasi::Graphics {
    class GraphicsDevice;
}

using namespace Quasi::Math;
using namespace Quasi::Graphics;

class LimboApp;

struct ScreenShake {
    fv2 offset;
    float amplitude = 0.0f;
    void Trigger(float amp);
    void Update(float dt);
};

struct PostEffect {
    float time = 0.0f;
    float innerRadius = 0, outerRadius = 0;
    fv2 aberrationOff = { 3 / 1920.0f, -2 / 1080.0f };
    fColor vignetteTint = { 0, 0 };
    enum State { USE_ANIM, NO_ANIM, MANUAL, CAPTURE, USE_EXPOSURE, DISABLED } state = NO_ANIM;
    bool vignetteForeground = false;
    Texture2D background;

    ScreenShake screenShake;

    FrameBuffer frameBuf;
    Texture2D screenTex;
    RenderBuffer depthBuffer;

    Shader shader;

    PostEffect() = default;

    void Init();

    void Use();
    void ApplyEffect();

    void Anim(LimboApp& app, float dt);
    void Reset();
    void EnterExposure();
    void Draw();
};
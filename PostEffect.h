#pragma once
#include "GLs/FrameBuffer.h"
#include "GLs/RenderBuffer.h"
#include "GLs/Shader.h"
#include "GLs/Texture.h"

using namespace Quasi::Math;
using namespace Quasi::Graphics;

struct PostEffect {
    FrameBuffer frameBuf;
    Texture2D screenTex;
    RenderBuffer depthBuffer;
    iv2 screenDim;

    Shader shader;

    PostEffect() = default;
    PostEffect(const iv2& screenDim, Shader&& shader);

    void SetToRenderTarget();
    void ApplyEffect();
};

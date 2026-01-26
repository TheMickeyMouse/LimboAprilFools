#pragma once
#include "GLs/FrameBuffer.h"
#include "GLs/RenderBuffer.h"
#include "GLs/Shader.h"
#include "GLs/Texture.h"

namespace Quasi::Graphics {
    struct PostEffect {
        FrameBuffer frameBuf;
        Texture2D screenTex, output;
        RenderBuffer depthBuffer;
        Math::iv2 screenDim;

        Shader shader;

        PostEffect() = default;
        PostEffect(const Math::iv2& screenDim, Shader&& shader);

        void SetToRenderTarget();
        void ApplyEffect();
    };
}

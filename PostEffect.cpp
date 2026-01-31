#include "PostEffect.h"

#include "glp.h"
#include "GraphicsDevice.h"

namespace Quasi::Graphics {
    PostEffect::PostEffect(const Math::iv2& screenDim, Shader&& shader) : screenDim(screenDim), shader(std::move(shader)) {
        frameBuf = FrameBuffer::New();
        depthBuffer = RenderBuffer::New(
            TextureIFormat::DEPTH, screenDim
        );
        screenTex = Texture2D::New(nullptr, screenDim, {
            .format = TextureFormat::RGBA, .internalformat = TextureIFormat::RGBA_32F, .type = GLTypeID::FLOAT,
        });
        output = Texture2D::New(nullptr, screenDim, {
            .format = TextureFormat::RGBA, .internalformat = TextureIFormat::RGBA_32F, .type = GLTypeID::FLOAT,
        });

        frameBuf.Bind();
        frameBuf.Attach(depthBuffer, AttachmentType::DEPTH);
        frameBuf.Unbind();
    }

    void PostEffect::SetToRenderTarget() {
        frameBuf.Bind();
        frameBuf.Attach(screenTex);
        GL::Viewport(0, 0, screenDim.x, screenDim.y);
    }

    void PostEffect::ApplyEffect() {
        screenTex.BindImageTexture(0, 0, Access::READ);
        output   .BindImageTexture(1, 0, Access::WRITE);

        shader.Bind();
        shader.ExecuteCompute(screenDim.x, screenDim.y);

        Render::MemoryBarrier(MemBarrier::TEXTURE_FETCH);

        frameBuf.Attach(output);
        frameBuf.BlitToScreen({ 0, screenDim }, { 0, screenDim });
    }
}

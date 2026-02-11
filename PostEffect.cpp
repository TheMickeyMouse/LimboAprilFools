#include "PostEffect.h"

#include "glp.h"
#include "GraphicsDevice.h"
#include "Mesh.h"

PostEffect::PostEffect(const iv2& screenDim, Shader&& shader) : screenDim(screenDim), shader(std::move(shader)) {
    frameBuf = FrameBuffer::New();
    depthBuffer = RenderBuffer::New(
        TextureIFormat::DEPTH, screenDim
    );
    screenTex = Texture2D::New(nullptr, screenDim, {
        .format = TextureFormat::RGBA, .internalformat = TextureIFormat::RGBA_32F, .type = GLTypeID::FLOAT,
    });

    frameBuf.Bind();
    frameBuf.Attach(depthBuffer, AttachmentType::DEPTH);
    frameBuf.Attach(screenTex);
    frameBuf.Unbind();
}

void PostEffect::SetToRenderTarget() {
    frameBuf.Bind();
    GL::Viewport(0, 0, screenDim.x, screenDim.y);
}

void PostEffect::ApplyEffect() {
    frameBuf.Unbind();
    Render::Clear();
    shader.Bind();
    shader.SetUniformTex("screenTex", screenTex, 5);
    Render::DrawScreenQuad(shader);
}

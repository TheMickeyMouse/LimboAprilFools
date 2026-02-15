#include "PostEffect.h"

#include "glp.h"
#include "GraphicsDevice.h"
#include "LimboApp.h"
#include "System.h"

void PostEffect::Init(LimboApp& app) {
    background = Texture2D::New(System::CaptureScreen(), {
        .format = TextureFormat::BGRA,
        .pixelated = true,
        .border = TextureBorder::CLAMP_TO_BORDER
    });

    frameBuf = FrameBuffer::New();
    depthBuffer = RenderBuffer::New(
        TextureIFormat::DEPTH, { (int)WIDTH, (int)HEIGHT }
    );
    screenTex = Texture2D::New(nullptr, { (int)WIDTH, (int)HEIGHT }, {
        .format = TextureFormat::RGBA, .internalformat = TextureIFormat::RGBA_32F, .type = GLTypeID::FLOAT,
    });

    shader = Shader::New(app.resources["post.glsl"].Transmute<char>().AsStr());

    frameBuf.Bind();
    frameBuf.Attach(depthBuffer, AttachmentType::DEPTH);
    frameBuf.Attach(screenTex);
    frameBuf.Unbind();
}

void PostEffect::Use() {
    if (state == DISABLED) return;
    frameBuf.Bind();
    GL::Viewport(0, 0, WIDTH, HEIGHT);
}

void PostEffect::ApplyEffect() {
    frameBuf.Unbind();
    Render::Clear();
    shader.Bind();
    shader.SetUniformTex("screenTex", screenTex, 5);
    Render::DrawScreenQuad(shader);
}

void PostEffect::Anim(LimboApp& app, float dt) {
    screenShake.Update(dt);

    if (state == MANUAL || state == CAPTURE || state == DISABLED) {
        // Debug::QInfo$("time = {}", time);
        return;
    }
    if (state == NO_ANIM) {
        app.globalScale = std::lerp(app.globalScale, 1.0f, 0.05f);
        return;
    }

    time += dt / 9.65f;

    const float t = std::min(time, 1.0f);

    innerRadius    = std::lerp(1.1f, 0.0f, t);
    outerRadius    = std::lerp(1.4f, 1.0f, t);
    vignetteTint.a = std::lerp(0.0f, 0.95f, t);
    aberrationOff  = { (3 + 10.0f * t) / WIDTH, (-2 - 6.0f * t) / HEIGHT };

    app.globalScale = std::lerp(1.0f, 1.2f, t);
    screenShake.Trigger(std::exp(16 * t - 15) * 75.0f);
}

void PostEffect::Reset() {
    state = NO_ANIM;
    time = 0;
    aberrationOff = { 3 / WIDTH, -2 / HEIGHT };
}

void PostEffect::CaptureBackground() {
    state = CAPTURE;
}

void PostEffect::DrawBackground(LimboApp& app) {
    app.canvas.DrawTexture(background, 0, { WIDTH, HEIGHT }, false);
}

void PostEffect::Draw() {
    if (state == DISABLED) return;
    shader.Bind();
    shader.SetUniformFloat("innerRadius",   innerRadius);
    shader.SetUniformFloat("outerRadius",   outerRadius);
    shader.SetUniformColor("vignetteTint",  vignetteTint);
    shader.SetUniformFv2  ("aberrationOff", aberrationOff);
    shader.SetUniformInt  ("vignetteOver",  vignetteForeground);
    shader.SetUniformTex  ("background",    background, 0);
    ApplyEffect();
    if (state == CAPTURE) {
        frameBuf.Bind();
        frameBuf.Attach(background);
        frameBuf.BlitFromScreen({ 0, { (int)WIDTH, (int)HEIGHT } }, { 0, { (int)WIDTH, (int)HEIGHT } });
        frameBuf.Bind();
        frameBuf.Attach(screenTex);
        FrameBuffer::Screen().Bind();
        state = DISABLED;
    }
}

void ScreenShake::Trigger(float amp) {
    amplitude = amp;
}
void ScreenShake::Update(float dt) {
    amplitude = std::max(amplitude - 240.0f * dt, 0.0f);
    if (amplitude != 0.0f) {
        auto& rand = GraphicsDevice::GetDeviceInstance().GetRand();
        offset = fv2::RandomInUnit(rand) * amplitude;
    }
}

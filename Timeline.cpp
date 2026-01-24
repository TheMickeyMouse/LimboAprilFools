#include "Timeline.h"
#include "LimboApp.h"

void Effect::Anim(LimboApp& app, float dt) {
    time += dt / duration;
}

CompoundEffect::CompoundEffect(Box<Effect> e, Box<Effect> f)
    : Effect(std::max(e->duration, f->duration)), e(std::move(e)), f(std::move(f)) {}

void CompoundEffect::Init(LimboApp& app) {
    Effect::Init(app);
    e->Init(app); f->Init(app);
}

void CompoundEffect::Anim(LimboApp& app, float dt) {
    Effect::Anim(app, dt);
    e->Anim(app, dt); f->Anim(app, dt);
}

void CompoundEffect::LateAnim(LimboApp& app, float dt) {
    Effect::LateAnim(app, dt);
    e->LateAnim(app, dt), f->LateAnim(app, dt);
}

void CompoundEffect::Finish(LimboApp& app) {
    Effect::Finish(app);
    e->Finish(app); f->Finish(app);
}

Timeline::Timeline(Vec<Box<Effect>> effects) : effects(std::move(effects)) {
    for (auto& effect : this->effects) {
        totalDuration += effect->duration;
    }
}

void Timeline::Anim(LimboApp& app, float dt) {
    if (currentEffect) currentEffect->Anim(app, dt);
}

void Timeline::LateAnim(LimboApp& app, float dt) {
    if (currentEffect) currentEffect->LateAnim(app, dt);

    if (!currentEffect || (frame < effects.Length() && currentEffect->Done())) {
        const float extraTime = currentEffect ? currentEffect->ExtraTime() : 0;
        if (currentEffect) currentEffect->Finish(app);
        app.ResetKeyPos();

        currentEffect = effects[frame].AsRef();
        currentEffect->AddTime(extraTime);
        currentEffect->Init(app);
        ++frame;
    }
}

#pragma once
#include "Utils/Vec.h"

using namespace Quasi;

class LimboApp;

class Effect {
public:
    float time = 0, duration = 0;

    explicit Effect(float dura) : time(-1.0f / (60.0f * dura)), duration(dura) {}
    virtual ~Effect() = default;
    virtual void Init(LimboApp& app) {}
    virtual void Anim(LimboApp& app, float dt);
    virtual void LateAnim(LimboApp& app, float dt) {}
    virtual void Finish(LimboApp& app) {}

    virtual bool Done() const { return time >= 1.0f; }
    float ExtraTime() const { return (time - 1.0f) * duration; }
    void AddTime(float dt) { time += dt / duration; }

    friend class Timeline;
};

class CompoundEffect : public Effect {
protected:
    Box<Effect> e, f;
public:
    CompoundEffect(Box<Effect> e, Box<Effect> f);
    virtual void Init(LimboApp& app);
    virtual void Anim(LimboApp& app, float dt);
    virtual void LateAnim(LimboApp& app, float dt);
    virtual void Finish(LimboApp& app);
};

class Timeline {
    Vec<Box<Effect>> effects;
    usize frame = 0;
    OptRef<Effect> currentEffect = nullptr;
public:
    float time = 0.0f, totalDuration = 0.0f;

    Timeline() = default;
    explicit Timeline(Vec<Box<Effect>> effects);

    void Anim(LimboApp& app, float dt);
    void LateAnim(LimboApp& app, float dt);
};
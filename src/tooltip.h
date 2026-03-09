#ifndef MONSTRO_INTERNAL_TOOLTIP_H
#define MONSTRO_INTERNAL_TOOLTIP_H

#include <cstdint>

namespace monstro {

struct Context;

struct TooltipState {
    const char* myText = nullptr;
    uint64_t myHoveredId = 0;
    float myHoverTimer = 0.0f;
    float myHoverDelay = 0.5f;
    float myStillnessThreshold = 2.0f;
    bool myActive = false;

    void beginFrame();
    void endFrame(Context* ctx);
    void update(uint64_t hoveredId, float deltaTime,
                float mouseDeltaX, float mouseDeltaY,
                bool mousePressed, const char* text);
};

} // namespace monstro

#endif

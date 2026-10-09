#ifndef MONSTRO_INTERNAL_TOOLTIP_H
#define MONSTRO_INTERNAL_TOOLTIP_H

#include "constants.h"
#include <cstdint>

namespace monstro
{

struct Context;

// The tooltip shown at the end of the frame. Its text is copied when the widget
// asks for it, because the caller's string need not outlive that call.
struct TooltipState
{
   char myText[MONSTRO_MAX_TOOLTIP_TEXT] = {};
   bool myHasText = false;
   uint64_t myHoveredId = 0;
   float myHoverTimer = 0.0f;
   float myHoverDelay = 0.5f;
   float myStillnessThreshold = 2.0f;
   bool myActive = false;

   void beginFrame();
   void endFrame(Context *ctx);
   void update(uint64_t hoveredId, float deltaTime, float mouseDeltaX, float mouseDeltaY, bool mousePressed,
               const char *text);

 private:
   void setText(const char *text);
};

} // namespace monstro

#endif

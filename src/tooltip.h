#ifndef MONSTRO_INTERNAL_TOOLTIP_H
#define MONSTRO_INTERNAL_TOOLTIP_H

#include "constants.h"
#include <cstdint>

namespace monstro
{

struct Context;

// The tooltip shown at the end of the frame. Its text is copied into the
// frame's canvas when the widget asks for it, because the caller's string need
// not outlive that call; the hover timing is kept between frames.
struct TooltipState
{
   uint64_t myHoveredId = 0;
   float myHoverTimer = 0.0f;
   float myHoverDelay = 0.5f;
   float myStillnessThreshold = 2.0f;
   bool myActive = false;

   void endFrame(Context *ctx);
   void update(Context *ctx, const char *text);
};

} // namespace monstro

#endif

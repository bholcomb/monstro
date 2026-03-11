#include "context.h"
#include <cmath>

namespace monstro
{

void TooltipState::beginFrame() { myText = nullptr; }

void TooltipState::endFrame(Context *ctx)
{
   if (myText == nullptr)
   {
      myHoveredId = 0;
      myHoverTimer = 0.0f;
      myActive = false;
      return;
   }

   if (!myActive)
      return;

   MonstroFont *font = ctx->style.font;
   if (!font || !myText)
      return;

   MonstroVec2 text_size = monstro_font_measure_text(font, myText, 0.0f);
   float pad_x = ctx->style.tooltip.padding[0];
   float pad_y = ctx->style.tooltip.padding[1];
   float win_w = text_size.x + pad_x * 2.0f;
   float win_h = text_size.y + pad_y * 2.0f;

   float tip_x = ctx->mouse.pos_x + 4.0f;
   float tip_y = ctx->mouse.pos_y + 12.0f;

   if (tip_x + win_w > ctx->display_width)
      tip_x = ctx->display_width - win_w;
   if (tip_y + win_h > ctx->display_height)
      tip_y = ctx->mouse.pos_y - win_h - 4.0f;
   if (tip_x < 0.0f)
      tip_x = 0.0f;
   if (tip_y < 0.0f)
      tip_y = 0.0f;

   monstro_set_next_window_pos(ctx, tip_x, tip_y, MONSTRO_COND_ALWAYS);

   MonstroWindowFlags flags = (MonstroWindowFlags)(MONSTRO_WINDOW_TOOLTIP | MONSTRO_WINDOW_AUTO_RESIZE |
                                                   MONSTRO_WINDOW_BACKGROUND | MONSTRO_WINDOW_BORDERS);

   if (monstro_begin_window(ctx, "tooltip##", nullptr, flags))
   {
      monstro_label(ctx, myText);
      monstro_end_window(ctx);
   }
}

void TooltipState::update(uint64_t hoveredId, float deltaTime, float mouseDeltaX, float mouseDeltaY, bool mousePressed,
                          const char *text)
{
   if (!text || text[0] == '\0')
      return;

   if (mousePressed)
   {
      myHoverTimer = 0.0f;
      myActive = false;
      myText = text;
      return;
   }

   if (hoveredId != myHoveredId)
   {
      myHoveredId = hoveredId;
      myHoverTimer = 0.0f;
      myActive = false;
      myText = text;
      return;
   }

   float movement = fabsf(mouseDeltaX) + fabsf(mouseDeltaY);
   if (movement > myStillnessThreshold)
   {
      myHoverTimer = 0.0f;
      myActive = false;
      myText = text;
      return;
   }

   myHoverTimer += deltaTime;
   myText = text;

   if (myHoverTimer >= myHoverDelay)
   {
      myActive = true;
   }
}

} // namespace monstro

extern "C"
{

void monstro_tooltip(MonstroContext *ctx, const char *text)
{
   if (!ctx || !text || text[0] == '\0')
      return;
   ctx->tooltip.update(ctx->hovered_id, ctx->delta_time, ctx->mouse.delta_x, ctx->mouse.delta_y,
                       ctx->mouse.anyButtonDown(), text);
}

} // extern "C"

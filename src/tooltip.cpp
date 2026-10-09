#include "context.h"
#include <cmath>
#include <cstring>

namespace monstro
{

static void set_text(Canvas *canvas, const char *text)
{
   size_t length = std::strlen(text);
   MONSTRO_REQUIRE(length < sizeof(canvas->tooltip_text), "tooltip text is %zu bytes; a tooltip holds at most %zu",
                   length, sizeof(canvas->tooltip_text) - 1);
   std::memcpy(canvas->tooltip_text, text, length + 1);
   canvas->has_tooltip = true;
}

void TooltipState::endFrame(Context *ctx)
{
   if (!ctx->canvas->has_tooltip)
   {
      myHoveredId = 0;
      myHoverTimer = 0.0f;
      myActive = false;
      return;
   }

   if (!myActive)
      return;

   MonstroFont *font = ctx->style().font;
   if (!font)
      return;

   const char *text = ctx->canvas->tooltip_text;
   MonstroVec2 text_size = monstro_font_measure_text(font, text, 0.0f);
   float pad_x = ctx->style().tooltip.padding[0];
   float pad_y = ctx->style().tooltip.padding[1];
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
      monstro_label(ctx, text);
      monstro_end_window(ctx);
   }
}

void TooltipState::update(Context *ctx, const char *text)
{
   if (text[0] == '\0')
      return;
   set_text(ctx->canvas, text);

   // Pressing, moving to another item or moving the mouse restarts the delay.
   float movement = fabsf(ctx->mouse.delta_x) + fabsf(ctx->mouse.delta_y);
   if (ctx->mouse.anyButtonDown() || ctx->hovered_id != myHoveredId || movement > myStillnessThreshold)
   {
      myHoveredId = ctx->hovered_id;
      myHoverTimer = 0.0f;
      myActive = false;
      return;
   }

   myHoverTimer += ctx->delta_time;
   if (myHoverTimer >= myHoverDelay)
      myActive = true;
}

} // namespace monstro

extern "C"
{

void monstro_tooltip(MonstroContext *ctx, const char *text)
{
   monstro::requireWindow(ctx, __func__, {MONSTRO_ARG(text)});
   ctx->tooltip.update(ctx, text);
}

} // extern "C"

#include "../context.h"
#include <cstdio>
#include <cstring>

static MonstroTabBarState *find_or_create_tab_bar(MonstroContext *ctx, uint64_t bar_id)
{
   for (MonstroTabBarState &existing : ctx->tab_bars)
   {
      if (existing.id == bar_id)
      {
         return &existing;
      }
   }

   MonstroTabBarState *state = &ctx->tab_bars.add();
   state->id = bar_id;
   return state;
}

bool monstro_begin_tab_bar(MonstroContext *ctx, const char *str_id, MonstroTabBarFlags flags)
{
   monstro::Window *win = monstro::requireWindow(ctx, __func__, {MONSTRO_ARG(str_id)});
   if (win->skip_items)
      return false;

   MonstroStyle *style = &ctx->style();

   uint64_t bar_id = win->getChildId(str_id);
   MonstroTabBarState *state = find_or_create_tab_bar(ctx, bar_id);

   state->last_frame_active = ctx->frame;
   state->flags = flags;
   state->first_tab = 0;
   state->selected_tab_present = false;
   state->content_active = false;
   // Tab bars nest: a tab's content may hold another tab bar.
   ctx->canvas->tab_bars.push(state);

   ctx->ids().push(str_id);

   MonstroVec2 pos = monstro_get_cursor_screen_pos(ctx);
   float bar_height = style->font ? monstro_font_get_line_height(style->font) : 16.0f;
   bar_height += style->tab.padding[1] * 2.0f;

   state->header_start_x = pos.x;
   state->header_start_y = pos.y;
   state->header_height = bar_height;

   MonstroRect bar_rect = {pos.x, pos.y, win->size.x - (pos.x - win->pos.x), bar_height};
   ctx->canvas->addRectFilled(bar_rect, monstro_color_to_u32(style->tab.background.data.color), 0, MONSTRO_CORNER_NONE);

   monstro_add_item(ctx, 0.0f, bar_height);

   return true;
}

void monstro_end_tab_bar(MonstroContext *ctx)
{
   monstro::Window *win = monstro::requireWindow(ctx, __func__);
   if (win->skip_items)
      return;

   MONSTRO_REQUIRE(!ctx->canvas->tab_bars.empty(), "monstro_end_tab_bar without monstro_begin_tab_bar (window '%s')",
                   win->name);
   MonstroTabBarState *bar = ctx->canvas->tab_bars.pop();
   bar->content_active = false;
   if ((bar->flags & MONSTRO_TAB_BAR_AUTO_SELECT) && !bar->selected_tab_present && bar->first_tab != 0)
   {
      bar->selected_tab = bar->first_tab;
   }

   ctx->ids().pop();
}

bool monstro_begin_tab_item(MonstroContext *ctx, const char *label, bool *p_open)
{
   monstro::Window *win = monstro::requireWindow(ctx, __func__, {MONSTRO_ARG(label)});
   if (win->skip_items)
      return false;

   MONSTRO_REQUIRE(!ctx->canvas->tab_bars.empty(), "monstro_begin_tab_item outside a tab bar (window '%s')", win->name);
   MonstroTabBarState *bar = ctx->canvas->tab_bars.top();

   MonstroStyle *style = &ctx->style();
   MonstroFont *font = style->font;
   monstro::Canvas *canvas = ctx->canvas;

   uint64_t tab_id = win->getItemId(label);
   const char *display = monstro_display_text(label);
   if (bar->first_tab == 0)
   {
      bar->first_tab = tab_id;
   }

   if (bar->selected_tab == 0)
   {
      bar->selected_tab = tab_id;
   }

   bool is_selected = (bar->selected_tab == tab_id);
   if (is_selected)
   {
      bar->selected_tab_present = true;
   }

   MonstroVec2 text_size = monstro_font_measure_text(font, display, 0.0f);
   if (text_size.x == 0.0f)
      text_size.x = (float)strlen(display) * 8.0f;
   if (text_size.y == 0.0f)
      text_size.y = 16.0f;

   float tab_w = text_size.x + style->tab.padding[0] * 2.0f;
   float tab_h = bar->header_height;

   if (p_open)
   {
      tab_w += text_size.y;
   }

   float tab_x = bar->header_start_x;
   float tab_y = bar->header_start_y;
   MonstroRect tab_rect = {tab_x, tab_y, tab_w, tab_h};

   bar->header_start_x += tab_w + style->tab.spacing[0];

   bool hovered = ctx->hoverItem(tab_id, tab_rect);
   if (hovered && monstro_is_mouse_clicked(ctx, MONSTRO_MOUSE_BUTTON_LEFT, false))
   {
      bar->selected_tab = tab_id;
      is_selected = true;
   }

   MonstroStyleButton *tab_btn = &style->tab.tab_maximize_button;
   uint32_t bg_color;
   uint32_t text_color;

   if (is_selected)
   {
      bg_color = monstro_color_to_u32(tab_btn->active.data.color);
      text_color = monstro_color_to_u32(tab_btn->text_active);
   }
   else if (hovered)
   {
      bg_color = monstro_color_to_u32(tab_btn->hover.data.color);
      text_color = monstro_color_to_u32(tab_btn->text_hover);
   }
   else
   {
      bg_color = monstro_color_to_u32(tab_btn->normal.data.color);
      text_color = monstro_color_to_u32(tab_btn->text_normal);
   }

   canvas->addRectFilled(tab_rect, bg_color, style->tab.rounding, MONSTRO_CORNER_TOP);

   if (is_selected && style->tab.border > 0.0f)
   {
      MonstroVec2 p1 = {tab_rect.x, tab_rect.y + tab_rect.h};
      MonstroVec2 p2 = {tab_rect.x + tab_rect.w, tab_rect.y + tab_rect.h};
      uint32_t accent = monstro_color_to_u32(ctx->style().tab.border_color);
      if ((accent & 0xFF000000) == 0)
      {
         accent = monstro_color_to_u32(tab_btn->active.data.color);
      }
      canvas->addLine(p1, p2, accent, style->tab.border);
   }

   MonstroRect label_rect = {tab_rect.x + style->tab.padding[0], tab_rect.y + style->tab.padding[1], text_size.x,
                             text_size.y};

   if (font)
   {
      canvas->addTextEx(font, label_rect, text_color, display, MONSTRO_ALIGN_LEFT);
   }

   if (p_open)
   {
      float close_size = text_size.y;
      MonstroRect close_rect = {tab_rect.x + tab_rect.w - close_size - style->tab.padding[0] * 0.5f,
                                tab_rect.y + style->tab.padding[1], close_size, close_size};

      // The close button sits on its tab and covers it.
      ctx->ids().push(tab_id);
      uint64_t close_id = ctx->ids().getId("close");
      ctx->ids().pop();
      bool close_hovered = ctx->hoverItem(close_id, close_rect);

      if (close_hovered)
      {
         canvas->addRectFilled(close_rect, monstro_color_to_u32(tab_btn->hover.data.color), style->tab.rounding,
                               MONSTRO_CORNER_ALL);
         if (monstro_is_mouse_clicked(ctx, MONSTRO_MOUSE_BUTTON_LEFT, false))
         {
            *p_open = false;
         }
      }

      float cx = close_rect.x + close_size * 0.25f;
      float cy = close_rect.y + close_size * 0.25f;
      float cs = close_size * 0.5f;
      canvas->addLine({cx, cy}, {cx + cs, cy + cs}, text_color, 1.0f);
      canvas->addLine({cx + cs, cy}, {cx, cy + cs}, text_color, 1.0f);
   }

   if (!is_selected)
   {
      return false;
   }

   bar->content_active = true;
   return true;
}

void monstro_end_tab_item(MonstroContext *ctx)
{
   monstro::Window *win = monstro::requireWindow(ctx, __func__);
   if (win->skip_items)
      return;
   MONSTRO_REQUIRE(!ctx->canvas->tab_bars.empty(), "monstro_end_tab_item outside a tab bar (window '%s')", win->name);
   ctx->canvas->tab_bars.top()->content_active = false;
}

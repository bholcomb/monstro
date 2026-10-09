#include "../context.h"
#include <cmath>
#include <cstring>

bool monstro_begin_combo(MonstroContext *ctx, const char *label, const char *preview_value, const char *tooltip)
{
   monstro::requireWindow(ctx, __func__, {MONSTRO_ARG(label)});

   monstro::Window *win = ctx->current_window;
   if (win->skip_items)
      return false;

   MonstroStyle *style = &ctx->style();
   monstro::Canvas *canvas = ctx->canvas;
   MonstroFont *font = style->font;

   uint64_t id = win->getChildId(label);
   const char *display = monstro_display_text(label);

   monstro_add_item(ctx, style->combo.content_padding[0], style->combo.content_padding[1]);

   MonstroVec2 pos = monstro_get_cursor_screen_pos(ctx);

   MonstroVec2 label_size = {0.0f, 0.0f};
   if (display[0] != '\0')
   {
      label_size = monstro_font_measure_text(font, display, 0.0f);
      if (label_size.x == 0.0f)
         label_size.x = (float)strlen(display) * 8.0f;
      if (label_size.y == 0.0f)
         label_size.y = 16.0f;
   }

   MonstroVec2 preview_size = {0.0f, 0.0f};
   if (preview_value)
   {
      preview_size = monstro_font_measure_text(font, preview_value, 0.0f);
      if (preview_size.x == 0.0f)
         preview_size.x = (float)strlen(preview_value) * 8.0f;
   }
   if (preview_size.y == 0.0f)
      preview_size.y = 16.0f;

   float pad_x = style->combo.content_padding[0];
   float pad_y = style->combo.content_padding[1];
   float button_width = preview_size.y;

   float combo_width = fmaxf(preview_size.x + pad_x * 2.0f + button_width + style->combo.spacing[0], 100.0f);
   float combo_height = preview_size.y + pad_y * 2.0f;

   float label_spacing = label_size.x > 0 ? style->combo.spacing[0] : 0.0f;
   monstro::Layout *layout = win->getCurrentLayout();
   if (layout && layout->myAvailableWidth > 0.0f)
   {
      float avail = layout->myAvailableWidth - label_size.x - label_spacing;
      if (avail > combo_width)
         combo_width = avail;
   }
   float total_width = label_size.x + label_spacing + combo_width;
   float total_height = fmaxf(label_size.y, combo_height);

   float combo_x = pos.x + label_size.x + label_spacing;
   MonstroRect combo_rect = {combo_x, pos.y, combo_width, combo_height};

   bool hovered = false;
   bool pressed = false;

   if (!ctx->disabled && win == ctx->hovered_window)
   {
      float mx = ctx->mouse.pos_x;
      float my = ctx->mouse.pos_y;
      hovered = mx >= combo_rect.x && mx < combo_rect.x + combo_rect.w && my >= combo_rect.y &&
                my < combo_rect.y + combo_rect.h;

      if (hovered)
      {
         ctx->hovered_id = id;
         if (monstro_is_mouse_clicked(ctx, MONSTRO_MOUSE_BUTTON_LEFT, false))
         {
            pressed = true;
         }
      }
   }

   bool opened = monstro_is_popup_open(ctx, label);

   MonstroStyleItem *bg_style = &style->combo.normal;
   MonstroColor label_color = style->combo.label_normal;

   if (opened)
   {
      bg_style = &style->combo.active;
      label_color = style->combo.label_active;
   }
   else if (hovered)
   {
      bg_style = &style->combo.hover;
      label_color = style->combo.label_hover;
   }

   if (bg_style->type == MONSTRO_STYLE_ITEM_COLOR)
   {
      canvas->addRectFilled(combo_rect, monstro_color_to_u32(bg_style->data.color), style->combo.rounding,
                            MONSTRO_CORNER_ALL);
   }

   canvas->addRect(combo_rect, monstro_color_to_u32(style->combo.border_color), style->combo.rounding,
                   MONSTRO_CORNER_ALL, style->combo.border);

   if (label_size.x > 0)
   {
      MonstroVec2 label_pos = {pos.x, pos.y + (total_height - label_size.y) * 0.5f};
      canvas->addText(font, label_pos, monstro_color_to_u32(style->text.color), display);
   }

   if (preview_value)
   {
      MonstroVec2 preview_pos = {combo_x + pad_x, pos.y + (combo_height - preview_size.y) * 0.5f};
      canvas->addText(font, preview_pos, monstro_color_to_u32(label_color), preview_value);
   }

   float arrow_x = combo_x + combo_width - button_width - pad_x * 0.5f;
   float arrow_y = pos.y + combo_height * 0.5f;
   float arrow_size = button_width * 0.4f;

   MonstroVec2 p1 = {arrow_x, arrow_y - arrow_size * 0.5f};
   MonstroVec2 p2 = {arrow_x + arrow_size, arrow_y - arrow_size * 0.5f};
   MonstroVec2 p3 = {arrow_x + arrow_size * 0.5f, arrow_y + arrow_size * 0.5f};
   canvas->addTriangleFilled(p1, p2, p3, monstro_color_to_u32(label_color));

   monstro_add_item(ctx, total_width, total_height);

   MonstroRect widget_rect = {pos.x, pos.y, total_width, total_height};
   monstro_draw_disabled_overlay(ctx, widget_rect);
   monstro_set_last_item(ctx, id, widget_rect, hovered, opened, pressed, false);

   if (pressed && !opened)
   {
      monstro_open_popup(ctx, label);
      return false;
   }

   // A press on the open combo closes its list; the combo itself is not in a popup.
   if (pressed && opened)
   {
      monstro::closePopup(ctx, id);
      return false;
   }

   if (opened)
   {
      float popup_x = combo_x;
      float popup_y = pos.y + combo_height;

      monstro_set_next_window_pos(ctx, popup_x, popup_y, MONSTRO_COND_ALWAYS);
      monstro_set_next_window_size(ctx, combo_width, 0.0f, MONSTRO_COND_ALWAYS);

      opened = monstro_begin_popup(ctx, label, (MonstroWindowFlags)0);

      if (opened)
      {
         monstro_begin_layout(ctx, MONSTRO_LAYOUT_VERTICAL);
      }
   }

   if (hovered && tooltip)
   {
      monstro_tooltip(ctx, tooltip);
   }

   return opened;
}

void monstro_end_combo(MonstroContext *ctx)
{
   monstro::requireWindow(ctx, __func__);

   monstro_end_layout(ctx);
   monstro_end_popup(ctx);
}

bool monstro_combo(MonstroContext *ctx, const char *label, int *current_item, const char *const items[],
                   int items_count, const char *tooltip)
{
   monstro::requireWindow(ctx, __func__, {MONSTRO_ARG(label), MONSTRO_ARG(current_item), MONSTRO_ARG(items)});
   MONSTRO_REQUIRE(items_count > 0, "monstro_combo: items_count is %d; a combo needs at least one item", items_count);

   int selected = *current_item;
   if (selected < 0 || selected >= items_count)
      selected = 0;

   const char *preview = items[selected];

   bool value_changed = false;

   if (monstro_begin_combo(ctx, label, preview, tooltip))
   {
      for (int i = 0; i < items_count; i++)
      {
         bool is_selected = (i == *current_item);

         if (monstro_selectable(ctx, items[i], &is_selected, MONSTRO_SELECTABLE_NONE, nullptr))
         {
            if (i != *current_item)
            {
               *current_item = i;
               value_changed = true;
            }
         }
      }

      monstro_end_combo(ctx);
   }

   return value_changed;
}

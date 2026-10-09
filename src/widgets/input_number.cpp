#include "../context.h"
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

static bool spinner_button(MonstroContext *ctx, monstro::Canvas *canvas, MonstroRect rect, const char *symbol,
                           MonstroStyleButton *bs, MonstroFont *font, uint64_t id)
{
   bool hovered = ctx->hoverItem(id, rect);
   bool pressed = false;
   if (hovered && monstro_is_mouse_clicked(ctx, MONSTRO_MOUSE_BUTTON_LEFT, false))
   {
      ctx->setActiveId(id);
      pressed = true;
   }

   bool held = false;
   if (ctx->active_id == id)
   {
      if (!ctx->disabled && monstro_is_mouse_down(ctx, MONSTRO_MOUSE_BUTTON_LEFT))
      {
         held = true;
      }
      else
      {
         ctx->setActiveId(0);
      }
   }

   MonstroStyleItem *bg = &bs->normal;
   if (held)
      bg = &bs->active;
   else if (hovered)
      bg = &bs->hover;

   if (bg->type == MONSTRO_STYLE_ITEM_COLOR)
   {
      canvas->addRectFilled(rect, monstro_color_to_u32(bg->data.color), bs->rounding, MONSTRO_CORNER_ALL);
   }
   if (bs->border > 0.0f)
   {
      canvas->addRect(rect, monstro_color_to_u32(bs->border_color), bs->rounding, MONSTRO_CORNER_ALL, bs->border);
   }

   if (font && symbol)
   {
      MonstroColor tc = held ? bs->text_active : (hovered ? bs->text_hover : bs->text_normal);
      canvas->addTextEx(font, rect, monstro_color_to_u32(tc), symbol, MONSTRO_ALIGN_CENTER);
   }

   if (held && !pressed)
   {
      float down_time = ctx->mouse.buttons[MONSTRO_MOUSE_BUTTON_LEFT].down_time;
      if (down_time > ctx->keyboard.repeat_delay && ctx->keyboard.repeat_rate > 0.0f)
      {
         float time_since = down_time - ctx->keyboard.repeat_delay;
         int count = (int)(time_since / ctx->keyboard.repeat_rate);
         float prev = time_since - ctx->delta_time;
         int prev_count = prev > 0.0f ? (int)(prev / ctx->keyboard.repeat_rate) : -1;
         if (count > prev_count)
            pressed = true;
      }
   }

   return pressed;
}

enum NumberKind
{
   NUMBER_INT,
   NUMBER_FLOAT,
   NUMBER_DOUBLE
};

// Ints format through an int argument (format "%d"); floats and doubles through a double.
static void format_number(char *out, size_t out_size, NumberKind kind, const char *format, double value)
{
   if (kind == NUMBER_INT)
      snprintf(out, out_size, format, (int)value);
   else
      snprintf(out, out_size, format, value);
   monstro_utf8_trim_partial(out);
}

static double parse_number(NumberKind kind, const char *text)
{
   if (kind == NUMBER_INT)
   {
      long long parsed = strtoll(text, nullptr, 10);
      return (double)(parsed < INT_MIN ? INT_MIN : parsed > INT_MAX ? INT_MAX : parsed);
   }
   double parsed = strtod(text, nullptr);
   return kind == NUMBER_FLOAT ? (double)(float)parsed : parsed;
}

static bool accepts_number_char(NumberKind kind, char c)
{
   if ((c >= '0' && c <= '9') || c == '-' || c == '+')
      return true;
   return kind != NUMBER_INT && (c == '.' || c == 'e' || c == 'E');
}

// The numeric field for all three value types. The value travels as a double,
// which holds every int, float and double exactly.
static bool number_impl(MonstroContext *ctx, const char *label, NumberKind kind, double *value, double step,
                        double step_fast, const char *format, const char *tooltip)
{
   monstro::Window *win = ctx->current_window;
   if (win->skip_items)
      return false;

   MonstroStyle *style = &ctx->style();
   monstro::Canvas *canvas = ctx->canvas;
   MonstroFont *font = style->font;
   MonstroStyleProperty *ps = &style->property;
   MonstroStyleEdit *es = &ps->edit;
   monstro::EditState *edit = &ctx->edit_state;

   uint64_t id = win->getChildId(label);
   const char *display = monstro_display_text(label);
   uint64_t dec_id = id ^ 0x100;
   uint64_t inc_id = id ^ 0x200;

   monstro_add_item(ctx, ps->padding[0], ps->padding[1]);

   float text_h = font ? monstro_font_get_line_height(font) : 16.0f;
   float label_w = font ? monstro_font_measure_text(font, display, 0.0f).x : (float)strlen(display) * 8.0f;

   bool has_buttons = step > 0.0f;
   float btn_w = has_buttons ? text_h : 0.0f;
   float field_w = 80.0f;
   monstro::Layout *layout = win->getCurrentLayout();
   if (layout && layout->myAvailableWidth > 0.0f)
   {
      float buttons_w = has_buttons ? btn_w * 2.0f : 0.0f;
      float avail = layout->myAvailableWidth - label_w - ps->padding[0] - buttons_w;
      if (avail > 0.0f)
         field_w = avail;
   }
   float field_h = text_h + es->padding[1] * 2.0f;
   float total_w = label_w + ps->padding[0] + (has_buttons ? btn_w : 0.0f) + field_w + (has_buttons ? btn_w : 0.0f);

   MonstroVec2 pos = monstro_get_cursor_screen_pos(ctx);
   float field_x = pos.x + label_w + ps->padding[0] + (has_buttons ? btn_w : 0.0f);
   MonstroRect field_rect = {field_x, pos.y, field_w, field_h};

   bool is_active = edit->isActiveFor(id);
   double old_value = *value;
   bool value_changed = false;

   // Spinner buttons
   if (has_buttons)
   {
      MonstroRect dec_rect = {pos.x + label_w + ps->padding[0], pos.y, btn_w, field_h};
      MonstroRect inc_rect = {field_x + field_w, pos.y, btn_w, field_h};

      double actual_step = step;
      if (ctx->active_id == dec_id || ctx->active_id == inc_id)
      {
         float down_time = ctx->mouse.buttons[MONSTRO_MOUSE_BUTTON_LEFT].down_time;
         if (step_fast > 0.0f && down_time > ctx->keyboard.repeat_delay * 3.0f)
         {
            actual_step = step_fast;
         }
      }

      if (spinner_button(ctx, canvas, dec_rect, "-", &ps->dec_button, font, dec_id))
      {
         *value -= actual_step;
         value_changed = true;
      }
      if (spinner_button(ctx, canvas, inc_rect, "+", &ps->inc_button, font, inc_id))
      {
         *value += actual_step;
         value_changed = true;
      }
   }

   // Field interaction
   bool hovered = ctx->hoverItem(id, field_rect);

   if (is_active)
   {
      if (ctx->disabled)
      {
         *value = parse_number(kind, edit->myNumberText);
         edit->deactivate();
         is_active = false;
         ctx->want_text_input = false;
         value_changed = true;
      }
      else
      {
         ctx->want_text_input = true;
         ctx->want_capture_keyboard = true;
         edit->myCursorBlinkTimer += ctx->delta_time;

         if (monstro_is_key_pressed(ctx, MONSTRO_KEY_ESCAPE, false))
         {
            edit->deactivate();
            is_active = false;
            ctx->want_text_input = false;
         }

         if (is_active && monstro_is_key_pressed(ctx, MONSTRO_KEY_ENTER, false))
         {
            *value = parse_number(kind, edit->myNumberText);
            edit->deactivate();
            is_active = false;
            ctx->want_text_input = false;
            value_changed = true;
         }

         if (is_active)
         {
            const char *input = ctx->keyboard.getInputText();
            if (input && input[0] != '\0')
            {
               bool accept = true;
               for (const char *c = input; *c; c++)
               {
                  if (!accepts_number_char(kind, *c))
                  {
                     accept = false;
                     break;
                  }
               }
               if (accept)
               {
                  edit->insertText(edit->myNumberText, sizeof(edit->myNumberText), input);
                  edit->myCursorBlinkTimer = 0.0f;
               }
            }

            if (monstro_is_key_pressed(ctx, MONSTRO_KEY_BACKSPACE, true))
            {
               if (edit->myHasSelection)
               {
                  edit->deleteSelection(edit->myNumberText);
               }
               else if (edit->myCursorPos > 0)
               {
                  int len = (int)strlen(edit->myNumberText);
                  memmove(edit->myNumberText + edit->myCursorPos - 1, edit->myNumberText + edit->myCursorPos,
                          len - edit->myCursorPos + 1);
                  edit->myCursorPos--;
                  edit->mySelectStart = edit->myCursorPos;
                  edit->mySelectEnd = edit->myCursorPos;
               }
               edit->myCursorBlinkTimer = 0.0f;
            }

            if (monstro_is_key_pressed(ctx, MONSTRO_KEY_LEFT_ARROW, true) && edit->myCursorPos > 0)
            {
               edit->myCursorPos--;
               edit->mySelectStart = edit->myCursorPos;
               edit->mySelectEnd = edit->myCursorPos;
               edit->myHasSelection = false;
               edit->myCursorBlinkTimer = 0.0f;
            }
            if (monstro_is_key_pressed(ctx, MONSTRO_KEY_RIGHT_ARROW, true))
            {
               int len = (int)strlen(edit->myNumberText);
               if (edit->myCursorPos < len)
                  edit->myCursorPos++;
               edit->mySelectStart = edit->myCursorPos;
               edit->mySelectEnd = edit->myCursorPos;
               edit->myHasSelection = false;
               edit->myCursorBlinkTimer = 0.0f;
            }
            if (monstro_is_key_pressed(ctx, MONSTRO_KEY_HOME, false))
            {
               edit->myCursorPos = 0;
               edit->mySelectStart = 0;
               edit->mySelectEnd = 0;
               edit->myHasSelection = false;
            }
            if (monstro_is_key_pressed(ctx, MONSTRO_KEY_END, false))
            {
               int len = (int)strlen(edit->myNumberText);
               edit->myCursorPos = len;
               edit->mySelectStart = len;
               edit->mySelectEnd = len;
               edit->myHasSelection = false;
            }
         }

         if (is_active && !hovered && monstro_is_mouse_clicked(ctx, MONSTRO_MOUSE_BUTTON_LEFT, false))
         {
            *value = parse_number(kind, edit->myNumberText);
            edit->deactivate();
            is_active = false;
            ctx->want_text_input = false;
            value_changed = true;
         }
      }
   }
   else
   {
      if (hovered && monstro_is_mouse_clicked(ctx, MONSTRO_MOUSE_BUTTON_LEFT, false))
      {
         format_number(edit->myNumberText, sizeof(edit->myNumberText), kind, format, *value);
         edit->activate(id, edit->myNumberText, true);
         is_active = true;
         ctx->want_text_input = true;
      }
   }

   // Drawing
   MonstroStyleItem *bg = is_active ? &es->active : (hovered ? &es->hover : &es->normal);
   if (bg->type == MONSTRO_STYLE_ITEM_COLOR)
   {
      canvas->addRectFilled(field_rect, monstro_color_to_u32(bg->data.color), ps->rounding, MONSTRO_CORNER_ALL);
   }
   if (ps->border > 0.0f)
   {
      canvas->addRect(field_rect, monstro_color_to_u32(ps->border_color), ps->rounding, MONSTRO_CORNER_ALL, ps->border);
   }

   MonstroRect text_clip = {field_rect.x + es->padding[0], field_rect.y, field_rect.w - es->padding[0] * 2.0f,
                            field_rect.h};
   canvas->pushClipRect(text_clip);

   if (font)
   {
      char value_text[64];
      if (is_active)
      {
         snprintf(value_text, sizeof(value_text), "%s", edit->myNumberText);
      }
      else
      {
         format_number(value_text, sizeof(value_text), kind, format, *value);
      }

      MonstroColor text_col = is_active ? es->text_active : (hovered ? es->text_hover : es->text_normal);
      float ty = field_rect.y + (field_rect.h - text_h) * 0.5f;

      if (is_active)
      {
         MonstroVec2 tpos = {field_rect.x + es->padding[0], ty};
         canvas->addText(font, tpos, monstro_color_to_u32(text_col), value_text);

         bool blink_on = fmodf(edit->myCursorBlinkTimer, 1.0f) < 0.5f;
         if (blink_on)
         {
            char tmp[64];
            int clen = edit->myCursorPos < (int)sizeof(tmp) - 1 ? edit->myCursorPos : (int)sizeof(tmp) - 1;
            memcpy(tmp, value_text, clen);
            tmp[clen] = '\0';
            float cx = field_rect.x + es->padding[0] + monstro_font_measure_text(font, tmp, 0.0f).x;
            MonstroVec2 c1 = {cx, field_rect.y + 2.0f};
            MonstroVec2 c2 = {cx, field_rect.y + field_rect.h - 2.0f};
            canvas->addLine(c1, c2, monstro_color_to_u32(es->cursor_normal),
                            es->cursor_size > 0.0f ? es->cursor_size : 1.0f);
         }
      }
      else
      {
         canvas->addTextEx(font, field_rect, monstro_color_to_u32(text_col), value_text, MONSTRO_ALIGN_CENTER);
      }
   }

   canvas->popClipRect();

   // Label
   if (font && label[0] != '#')
   {
      MonstroRect label_rect = {pos.x, pos.y, label_w, field_h};
      MonstroColor lc = hovered ? ps->label_hover : ps->label_normal;
      canvas->addTextEx(font, label_rect, monstro_color_to_u32(lc), display, MONSTRO_ALIGN_DEFAULT);
   }

   monstro_add_item(ctx, total_w, field_h);

   MonstroRect widget_rect = {pos.x, pos.y, total_w, field_h};
   monstro_draw_disabled_overlay(ctx, widget_rect);
   bool held = is_active || (ctx->active_id == dec_id || ctx->active_id == inc_id);
   monstro_set_last_item(ctx, id, widget_rect, hovered, held, false, value_changed);

   if (hovered && tooltip)
   {
      monstro_tooltip(ctx, tooltip);
   }

   return value_changed || *value != old_value;
}

bool monstro_input_float(MonstroContext *ctx, const char *label, float *value, float step, float step_fast,
                         const char *format, const char *tooltip)
{
   monstro::requireWindow(ctx, __func__, {MONSTRO_ARG(label), MONSTRO_ARG(value)});

   double v = *value;
   bool changed = number_impl(ctx, label, NUMBER_FLOAT, &v, step, step_fast, format ? format : "%.3f", tooltip);
   *value = (float)v;
   return changed;
}

bool monstro_input_int(MonstroContext *ctx, const char *label, int *value, int step, int step_fast, const char *tooltip)
{
   monstro::requireWindow(ctx, __func__, {MONSTRO_ARG(label), MONSTRO_ARG(value)});

   double v = *value;
   bool changed = number_impl(ctx, label, NUMBER_INT, &v, step, step_fast, "%d", tooltip);
   *value = v < INT_MIN ? INT_MIN : v > INT_MAX ? INT_MAX : (int)v;
   return changed;
}

bool monstro_input_double(MonstroContext *ctx, const char *label, double *value, double step, double step_fast,
                          const char *format, const char *tooltip)
{
   monstro::requireWindow(ctx, __func__, {MONSTRO_ARG(label), MONSTRO_ARG(value)});

   return number_impl(ctx, label, NUMBER_DOUBLE, value, step, step_fast, format ? format : "%.6f", tooltip);
}

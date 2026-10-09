#include "../context.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

static float measure_substring(MonstroFont *font, const char *text, int start, int end)
{
   if (!font || !text || start >= end)
      return 0.0f;
   char tmp[1024];
   int len = end - start;
   if (len >= (int)sizeof(tmp))
      len = (int)sizeof(tmp) - 1;
   memcpy(tmp, text + start, len);
   tmp[len] = '\0';
   return monstro_font_measure_text(font, tmp, 0.0f).x;
}

static bool is_ctrl_pressed(MonstroContext *ctx) { return (ctx->keyboard.getMods() & MONSTRO_MOD_CTRL) != 0; }

static bool is_shift_pressed(MonstroContext *ctx) { return (ctx->keyboard.getMods() & MONSTRO_MOD_SHIFT) != 0; }

static bool key_pressed(MonstroContext *ctx, MonstroKey key) { return monstro_is_key_pressed(ctx, key, true); }

static int find_click_position(MonstroFont *font, const char *text, int text_len, float click_x, float field_x,
                               float pad_x)
{
   float offset = click_x - field_x - pad_x;
   if (offset <= 0.0f)
      return 0;

   for (int i = 1; i <= text_len; i++)
   {
      float w = measure_substring(font, text, 0, i);
      if (w >= offset)
      {
         float prev_w = measure_substring(font, text, 0, i - 1);
         if (offset - prev_w < w - offset)
            return i - 1;
         return i;
      }
   }
   return text_len;
}

static void clamp_edit_state_to_buffer(monstro::EditState *edit, const char *buf)
{
   if (!edit || !buf)
      return;

   int len = (int)strlen(buf);
   if (edit->myCursorPos < 0)
      edit->myCursorPos = 0;
   else if (edit->myCursorPos > len)
      edit->myCursorPos = len;

   if (edit->mySelectStart < 0)
      edit->mySelectStart = 0;
   else if (edit->mySelectStart > len)
      edit->mySelectStart = len;

   if (edit->mySelectEnd < 0)
      edit->mySelectEnd = 0;
   else if (edit->mySelectEnd > len)
      edit->mySelectEnd = len;

   edit->myHasSelection = edit->mySelectStart != edit->mySelectEnd;
}

bool monstro_input_text(MonstroContext *ctx, const char *label, char *buf, size_t buf_size, MonstroInputTextFlags flags,
                        const char *tooltip)
{
   monstro::requireWindow(ctx, __func__);
   if (!label || !buf)
      return false;

   monstro::Window *win = ctx->current_window;
   if (win->skip_items)
      return false;

   MonstroStyle *style = &ctx->style;
   monstro::Canvas *canvas = &win->canvas;
   MonstroFont *font = style->font;
   MonstroStyleEdit *es = &style->edit;
   monstro::EditState *edit = &ctx->edit_state;

   uint64_t id = win->getChildId(label);
   const char *display = monstro_display_text(label);
   bool is_read_only = (flags & MONSTRO_INPUT_TEXT_READ_ONLY) != 0;
   bool is_password = (flags & MONSTRO_INPUT_TEXT_PASSWORD) != 0;
   bool enter_returns = (flags & MONSTRO_INPUT_TEXT_ENTER_RETURNS) != 0;
   bool auto_select = (flags & MONSTRO_INPUT_TEXT_AUTO_SELECT_ALL) != 0;

   monstro_add_item(ctx, es->padding[0], es->padding[1]);

   float text_h = font ? monstro_font_get_line_height(font) : 16.0f;
   float label_w = font ? monstro_font_measure_text(font, display, 0.0f).x : (float)strlen(display) * 8.0f;

   float field_w = 120.0f;
   monstro::Layout *layout = win->getCurrentLayout();
   if (layout && layout->myAvailableWidth > 0.0f)
   {
      float avail = layout->myAvailableWidth - label_w - es->padding[0];
      if (avail > 0.0f)
         field_w = avail;
   }
   float field_h = text_h + es->padding[1] * 2.0f;
   float total_w = label_w + es->padding[0] + field_w;

   MonstroVec2 pos = monstro_get_cursor_screen_pos(ctx);
   MonstroRect field_rect = {pos.x + label_w + es->padding[0], pos.y, field_w, field_h};

   int text_len = (int)strlen(buf);
   bool is_active = edit->isActiveFor(id);
   bool value_changed = false;
   bool submitted = false;

   if (is_active)
      clamp_edit_state_to_buffer(edit, buf);

   char display_buf[1024];
   if (is_password)
   {
      int dlen = text_len < (int)sizeof(display_buf) - 1 ? text_len : (int)sizeof(display_buf) - 1;
      memset(display_buf, '*', dlen);
      display_buf[dlen] = '\0';
   }
   else
   {
      int dlen = text_len < (int)sizeof(display_buf) - 1 ? text_len : (int)sizeof(display_buf) - 1;
      memcpy(display_buf, buf, dlen);
      display_buf[dlen] = '\0';
   }

   // Hit testing
   bool hovered = false;
   if (!ctx->disabled && win == ctx->hovered_window)
   {
      float mx = ctx->mouse.pos_x;
      float my = ctx->mouse.pos_y;
      hovered = mx >= field_rect.x && mx < field_rect.x + field_rect.w && my >= field_rect.y &&
                my < field_rect.y + field_rect.h;
      if (hovered)
      {
         ctx->hovered_id = id;
      }
   }

   if (hovered && monstro_is_mouse_clicked(ctx, MONSTRO_MOUSE_BUTTON_LEFT, false))
   {
      if (!is_active)
      {
         edit->activate(id, buf, buf_size, auto_select);
         is_active = true;
         ctx->want_text_input = true;
      }
      if (is_active && font)
      {
         int click_pos =
             find_click_position(font, display_buf, text_len, ctx->mouse.pos_x, field_rect.x, es->padding[0]);
         edit->myCursorPos = click_pos;
         if (!is_shift_pressed(ctx))
         {
            edit->mySelectStart = click_pos;
            edit->mySelectEnd = click_pos;
            edit->myHasSelection = false;
         }
         else
         {
            edit->mySelectEnd = click_pos;
            edit->myHasSelection = edit->mySelectStart != edit->mySelectEnd;
         }
      }
   }

   if (is_active && !hovered && monstro_is_mouse_clicked(ctx, MONSTRO_MOUSE_BUTTON_LEFT, false))
   {
      edit->deactivate();
      is_active = false;
      ctx->want_text_input = false;
   }

   // Keyboard input when active
   if (is_active)
   {
      if (ctx->disabled)
      {
         edit->deactivate();
         is_active = false;
         ctx->want_text_input = false;
      }
      else
      {
         ctx->want_text_input = true;
         ctx->want_capture_keyboard = true;
         edit->myCursorBlinkTimer += ctx->delta_time;

         if (key_pressed(ctx, MONSTRO_KEY_ESCAPE))
         {
            edit->deactivate();
            is_active = false;
            ctx->want_text_input = false;
         }

         if (is_active && key_pressed(ctx, MONSTRO_KEY_ENTER))
         {
            if (enter_returns)
            {
               submitted = true;
            }
            edit->deactivate();
            is_active = false;
            ctx->want_text_input = false;
         }

         if (is_active && !is_read_only)
         {
            const char *input = ctx->keyboard.getInputText();
            if (input && input[0] != '\0')
            {
               bool accept = true;
               if (flags & MONSTRO_INPUT_TEXT_CHARS_DECIMAL)
               {
                  for (const char *c = input; *c; c++)
                  {
                     if (!((*c >= '0' && *c <= '9') || *c == '.' || *c == '-' || *c == '+'))
                     {
                        accept = false;
                        break;
                     }
                  }
               }
               if (flags & MONSTRO_INPUT_TEXT_CHARS_NO_BLANK)
               {
                  for (const char *c = input; *c; c++)
                  {
                     if (*c == ' ')
                     {
                        accept = false;
                        break;
                     }
                  }
               }
               if (accept)
               {
                  edit->insertText(input);
                  value_changed = true;
                  edit->myCursorBlinkTimer = 0.0f;
               }
            }

            if (key_pressed(ctx, MONSTRO_KEY_BACKSPACE))
            {
               if (edit->myHasSelection)
               {
                  edit->deleteSelection();
                  value_changed = true;
               }
               else if (edit->myCursorPos > 0)
               {
                  text_len = (int)strlen(buf);
                  memmove(buf + edit->myCursorPos - 1, buf + edit->myCursorPos, text_len - edit->myCursorPos + 1);
                  edit->myCursorPos--;
                  edit->mySelectStart = edit->myCursorPos;
                  edit->mySelectEnd = edit->myCursorPos;
                  value_changed = true;
               }
               edit->myCursorBlinkTimer = 0.0f;
            }

            if (key_pressed(ctx, MONSTRO_KEY_DELETE))
            {
               if (edit->myHasSelection)
               {
                  edit->deleteSelection();
                  value_changed = true;
               }
               else
               {
                  text_len = (int)strlen(buf);
                  if (edit->myCursorPos < text_len)
                  {
                     memmove(buf + edit->myCursorPos, buf + edit->myCursorPos + 1, text_len - edit->myCursorPos);
                     value_changed = true;
                  }
               }
               edit->myCursorBlinkTimer = 0.0f;
            }
         }

         if (is_active)
         {
            bool shift = is_shift_pressed(ctx);

            if (key_pressed(ctx, MONSTRO_KEY_LEFT_ARROW))
            {
               if (edit->myCursorPos > 0)
                  edit->myCursorPos--;
               if (shift)
               {
                  edit->mySelectEnd = edit->myCursorPos;
                  edit->myHasSelection = edit->mySelectStart != edit->mySelectEnd;
               }
               else
               {
                  edit->mySelectStart = edit->myCursorPos;
                  edit->mySelectEnd = edit->myCursorPos;
                  edit->myHasSelection = false;
               }
               edit->myCursorBlinkTimer = 0.0f;
            }

            if (key_pressed(ctx, MONSTRO_KEY_RIGHT_ARROW))
            {
               text_len = (int)strlen(buf);
               if (edit->myCursorPos < text_len)
                  edit->myCursorPos++;
               if (shift)
               {
                  edit->mySelectEnd = edit->myCursorPos;
                  edit->myHasSelection = edit->mySelectStart != edit->mySelectEnd;
               }
               else
               {
                  edit->mySelectStart = edit->myCursorPos;
                  edit->mySelectEnd = edit->myCursorPos;
                  edit->myHasSelection = false;
               }
               edit->myCursorBlinkTimer = 0.0f;
            }

            if (key_pressed(ctx, MONSTRO_KEY_HOME))
            {
               edit->myCursorPos = 0;
               if (shift)
               {
                  edit->mySelectEnd = 0;
                  edit->myHasSelection = edit->mySelectStart != edit->mySelectEnd;
               }
               else
               {
                  edit->mySelectStart = 0;
                  edit->mySelectEnd = 0;
                  edit->myHasSelection = false;
               }
               edit->myCursorBlinkTimer = 0.0f;
            }

            if (key_pressed(ctx, MONSTRO_KEY_END))
            {
               text_len = (int)strlen(buf);
               edit->myCursorPos = text_len;
               if (shift)
               {
                  edit->mySelectEnd = text_len;
                  edit->myHasSelection = edit->mySelectStart != edit->mySelectEnd;
               }
               else
               {
                  edit->mySelectStart = text_len;
                  edit->mySelectEnd = text_len;
                  edit->myHasSelection = false;
               }
               edit->myCursorBlinkTimer = 0.0f;
            }

            if (is_ctrl_pressed(ctx) && key_pressed(ctx, MONSTRO_KEY_A))
            {
               edit->selectAll();
               edit->myCursorBlinkTimer = 0.0f;
            }
         }

         // Refresh display buf after edits
         text_len = (int)strlen(buf);
         if (is_password)
         {
            int dlen = text_len < (int)sizeof(display_buf) - 1 ? text_len : (int)sizeof(display_buf) - 1;
            memset(display_buf, '*', dlen);
            display_buf[dlen] = '\0';
         }
         else
         {
            int dlen = text_len < (int)sizeof(display_buf) - 1 ? text_len : (int)sizeof(display_buf) - 1;
            memcpy(display_buf, buf, dlen);
            display_buf[dlen] = '\0';
         }
      }
   }

   // Drawing
   MonstroStyleItem *bg = &es->normal;
   if (is_active)
      bg = &es->active;
   else if (hovered)
      bg = &es->hover;

   if (bg->type == MONSTRO_STYLE_ITEM_COLOR)
   {
      canvas->addRectFilled(field_rect, monstro_color_to_u32(bg->data.color), es->rounding, MONSTRO_CORNER_ALL);
   }
   if (es->border > 0.0f)
   {
      canvas->addRect(field_rect, monstro_color_to_u32(es->border_color), es->rounding, MONSTRO_CORNER_ALL, es->border);
   }

   MonstroRect text_clip = {field_rect.x + es->padding[0], field_rect.y, field_rect.w - es->padding[0] * 2.0f,
                            field_rect.h};
   canvas->pushClipRect(text_clip);

   float text_x = field_rect.x + es->padding[0];
   float text_y = field_rect.y + (field_rect.h - text_h) * 0.5f;

   float visible_w = field_rect.w - es->padding[0] * 2.0f;
   float scroll_offset = 0.0f;

   if (is_active && font)
   {
      float cursor_x = measure_substring(font, display_buf, 0, edit->myCursorPos);
      if (cursor_x - scroll_offset > visible_w)
      {
         scroll_offset = cursor_x - visible_w;
      }
      if (cursor_x - scroll_offset < 0.0f)
      {
         scroll_offset = cursor_x;
      }
   }

   // Selection highlight
   if (is_active && edit->myHasSelection && font)
   {
      int sel_s = edit->getSelectionStart();
      int sel_e = edit->getSelectionEnd();
      float sel_start_x = measure_substring(font, display_buf, 0, sel_s) - scroll_offset;
      float sel_end_x = measure_substring(font, display_buf, 0, sel_e) - scroll_offset;
      MonstroRect sel_rect = {text_x + sel_start_x, field_rect.y + 1.0f, sel_end_x - sel_start_x, field_rect.h - 2.0f};
      canvas->addRectFilled(sel_rect, monstro_color_to_u32(es->selected_normal), 0.0f, MONSTRO_CORNER_ALL);
   }

   // Text
   if (font && display_buf[0] != '\0')
   {
      MonstroColor text_col = is_active ? es->text_active : (hovered ? es->text_hover : es->text_normal);
      MonstroVec2 text_pos = {text_x - scroll_offset, text_y};
      canvas->addText(font, text_pos, monstro_color_to_u32(text_col), display_buf);
   }

   // Cursor
   if (is_active && font)
   {
      bool blink_on = fmodf(edit->myCursorBlinkTimer, 1.0f) < 0.5f;
      if (blink_on)
      {
         float cursor_x_pos = measure_substring(font, display_buf, 0, edit->myCursorPos) - scroll_offset;
         MonstroVec2 c1 = {text_x + cursor_x_pos, field_rect.y + 2.0f};
         MonstroVec2 c2 = {text_x + cursor_x_pos, field_rect.y + field_rect.h - 2.0f};
         canvas->addLine(c1, c2, monstro_color_to_u32(es->cursor_normal),
                         es->cursor_size > 0.0f ? es->cursor_size : 1.0f);
      }
   }

   canvas->popClipRect();

   if (font && display[0] != '\0')
   {
      MonstroRect label_rect = {pos.x, pos.y, label_w, field_h};
      canvas->addTextEx(font, label_rect, monstro_color_to_u32(style->text.color), display, MONSTRO_ALIGN_DEFAULT);
   }

   monstro_add_item(ctx, total_w, field_h);

   MonstroRect widget_rect = {pos.x, pos.y, total_w, field_h};
   monstro_draw_disabled_overlay(ctx, widget_rect);
   monstro_set_last_item(ctx, id, widget_rect, hovered, is_active, false, value_changed);

   if (hovered && tooltip)
   {
      monstro_tooltip(ctx, tooltip);
   }

   return enter_returns ? submitted : value_changed;
}

bool monstro_input_text_multiline(MonstroContext *ctx, const char *label, char *buf, size_t buf_size, float width,
                                  float height, MonstroInputTextFlags flags, const char *tooltip)
{
   monstro::requireWindow(ctx, __func__);
   if (!label || !buf)
      return false;

   monstro::Window *win = ctx->current_window;
   if (win->skip_items)
      return false;

   MonstroStyle *style = &ctx->style;
   monstro::Canvas *canvas = &win->canvas;
   MonstroFont *font = style->font;
   MonstroStyleEdit *es = &style->edit;
   monstro::EditState *edit = &ctx->edit_state;

   uint64_t id = win->getChildId(label);
   const char *display = monstro_display_text(label);
   bool is_read_only = (flags & MONSTRO_INPUT_TEXT_READ_ONLY) != 0;

   monstro_add_item(ctx, es->padding[0], es->padding[1]);

   float text_h = font ? monstro_font_get_line_height(font) : 16.0f;
   float label_w = font ? monstro_font_measure_text(font, display, 0.0f).x : (float)strlen(display) * 8.0f;

   float field_w = width > 0.0f ? width : 200.0f;
   if (width <= 0.0f)
   {
      monstro::Layout *layout = win->getCurrentLayout();
      if (layout && layout->myAvailableWidth > 0.0f)
      {
         float avail = layout->myAvailableWidth - label_w - es->padding[0];
         if (avail > 0.0f)
            field_w = avail;
      }
   }
   float field_h = height > 0.0f ? height : text_h * 4.0f + es->padding[1] * 2.0f;
   float total_w = label_w + es->padding[0] + field_w;

   MonstroVec2 pos = monstro_get_cursor_screen_pos(ctx);
   MonstroRect field_rect = {pos.x + label_w + es->padding[0], pos.y, field_w, field_h};

   int text_len = (int)strlen(buf);
   bool is_active = edit->isActiveFor(id);
   bool value_changed = false;

   if (is_active)
      clamp_edit_state_to_buffer(edit, buf);

   // Hit testing
   bool hovered = false;
   if (!ctx->disabled && win == ctx->hovered_window)
   {
      float mx = ctx->mouse.pos_x;
      float my = ctx->mouse.pos_y;
      hovered = mx >= field_rect.x && mx < field_rect.x + field_rect.w && my >= field_rect.y &&
                my < field_rect.y + field_rect.h;
      if (hovered)
         ctx->hovered_id = id;
   }

   if (hovered && monstro_is_mouse_clicked(ctx, MONSTRO_MOUSE_BUTTON_LEFT, false))
   {
      if (!is_active)
      {
         edit->activate(id, buf, buf_size, false);
         is_active = true;
         ctx->want_text_input = true;
      }
   }

   if (is_active && !hovered && monstro_is_mouse_clicked(ctx, MONSTRO_MOUSE_BUTTON_LEFT, false))
   {
      edit->deactivate();
      is_active = false;
      ctx->want_text_input = false;
   }

   if (is_active)
   {
      if (ctx->disabled)
      {
         edit->deactivate();
         is_active = false;
         ctx->want_text_input = false;
      }
      else
      {
         ctx->want_text_input = true;
         ctx->want_capture_keyboard = true;
         edit->myCursorBlinkTimer += ctx->delta_time;

         if (key_pressed(ctx, MONSTRO_KEY_ESCAPE))
         {
            edit->deactivate();
            is_active = false;
            ctx->want_text_input = false;
         }

         if (is_active && !is_read_only)
         {
            if (key_pressed(ctx, MONSTRO_KEY_ENTER))
            {
               edit->insertText("\n");
               value_changed = true;
               edit->myCursorBlinkTimer = 0.0f;
            }

            const char *input = ctx->keyboard.getInputText();
            if (input && input[0] != '\0')
            {
               edit->insertText(input);
               value_changed = true;
               edit->myCursorBlinkTimer = 0.0f;
            }

            if (key_pressed(ctx, MONSTRO_KEY_BACKSPACE))
            {
               if (edit->myHasSelection)
               {
                  edit->deleteSelection();
               }
               else if (edit->myCursorPos > 0)
               {
                  text_len = (int)strlen(buf);
                  memmove(buf + edit->myCursorPos - 1, buf + edit->myCursorPos, text_len - edit->myCursorPos + 1);
                  edit->myCursorPos--;
                  edit->mySelectStart = edit->myCursorPos;
                  edit->mySelectEnd = edit->myCursorPos;
               }
               value_changed = true;
               edit->myCursorBlinkTimer = 0.0f;
            }

            if (key_pressed(ctx, MONSTRO_KEY_DELETE))
            {
               if (edit->myHasSelection)
               {
                  edit->deleteSelection();
               }
               else
               {
                  text_len = (int)strlen(buf);
                  if (edit->myCursorPos < text_len)
                  {
                     memmove(buf + edit->myCursorPos, buf + edit->myCursorPos + 1, text_len - edit->myCursorPos);
                  }
               }
               value_changed = true;
               edit->myCursorBlinkTimer = 0.0f;
            }
         }

         if (is_active)
         {
            if (key_pressed(ctx, MONSTRO_KEY_LEFT_ARROW) && edit->myCursorPos > 0)
            {
               edit->myCursorPos--;
               edit->mySelectStart = edit->myCursorPos;
               edit->mySelectEnd = edit->myCursorPos;
               edit->myHasSelection = false;
               edit->myCursorBlinkTimer = 0.0f;
            }
            if (key_pressed(ctx, MONSTRO_KEY_RIGHT_ARROW))
            {
               text_len = (int)strlen(buf);
               if (edit->myCursorPos < text_len)
                  edit->myCursorPos++;
               edit->mySelectStart = edit->myCursorPos;
               edit->mySelectEnd = edit->myCursorPos;
               edit->myHasSelection = false;
               edit->myCursorBlinkTimer = 0.0f;
            }
            if (key_pressed(ctx, MONSTRO_KEY_HOME))
            {
               edit->myCursorPos = 0;
               edit->mySelectStart = 0;
               edit->mySelectEnd = 0;
               edit->myHasSelection = false;
               edit->myCursorBlinkTimer = 0.0f;
            }
            if (key_pressed(ctx, MONSTRO_KEY_END))
            {
               text_len = (int)strlen(buf);
               edit->myCursorPos = text_len;
               edit->mySelectStart = text_len;
               edit->mySelectEnd = text_len;
               edit->myHasSelection = false;
               edit->myCursorBlinkTimer = 0.0f;
            }
            if (is_ctrl_pressed(ctx) && key_pressed(ctx, MONSTRO_KEY_A))
            {
               edit->selectAll();
               edit->myCursorBlinkTimer = 0.0f;
            }
         }
      }
   }

   // Drawing
   MonstroStyleItem *bg = &es->normal;
   if (is_active)
      bg = &es->active;
   else if (hovered)
      bg = &es->hover;

   if (bg->type == MONSTRO_STYLE_ITEM_COLOR)
   {
      canvas->addRectFilled(field_rect, monstro_color_to_u32(bg->data.color), es->rounding, MONSTRO_CORNER_ALL);
   }
   if (es->border > 0.0f)
   {
      canvas->addRect(field_rect, monstro_color_to_u32(es->border_color), es->rounding, MONSTRO_CORNER_ALL, es->border);
   }

   MonstroRect text_clip = {field_rect.x + es->padding[0], field_rect.y + es->padding[1],
                            field_rect.w - es->padding[0] * 2.0f, field_rect.h - es->padding[1] * 2.0f};
   canvas->pushClipRect(text_clip);

   // Render lines
   text_len = (int)strlen(buf);
   float line_y = field_rect.y + es->padding[1];
   float line_h = text_h + es->row_padding;
   MonstroColor text_col = is_active ? es->text_active : (hovered ? es->text_hover : es->text_normal);

   const char *line_start = buf;
   while (*line_start || line_start == buf)
   {
      const char *line_end = line_start;
      while (*line_end && *line_end != '\n')
         line_end++;

      if (line_y + text_h > field_rect.y + field_rect.h)
         break;

      int line_len = (int)(line_end - line_start);
      if (line_len > 0 && font)
      {
         char line_buf[1024];
         int copy_len = line_len < (int)sizeof(line_buf) - 1 ? line_len : (int)sizeof(line_buf) - 1;
         memcpy(line_buf, line_start, copy_len);
         line_buf[copy_len] = '\0';
         MonstroVec2 tpos = {field_rect.x + es->padding[0], line_y};
         canvas->addText(font, tpos, monstro_color_to_u32(text_col), line_buf);
      }

      line_y += line_h;
      if (*line_end == '\n')
      {
         line_start = line_end + 1;
      }
      else
      {
         break;
      }
   }

   // Cursor for multiline (simplified: end of text)
   if (is_active && font)
   {
      bool blink_on = fmodf(edit->myCursorBlinkTimer, 1.0f) < 0.5f;
      if (blink_on)
      {
         // Find cursor line and column
         int cur_line = 0;
         int col = 0;
         for (int i = 0; i < edit->myCursorPos && i < text_len; i++)
         {
            if (buf[i] == '\n')
            {
               cur_line++;
               col = 0;
            }
            else
               col++;
         }
         float cy = field_rect.y + es->padding[1] + cur_line * line_h;

         char col_buf[1024];
         const char *ls = buf;
         for (int l = 0; l < cur_line; l++)
         {
            while (*ls && *ls != '\n')
               ls++;
            if (*ls == '\n')
               ls++;
         }
         int clen = col < (int)sizeof(col_buf) - 1 ? col : (int)sizeof(col_buf) - 1;
         memcpy(col_buf, ls, clen);
         col_buf[clen] = '\0';
         float cx = field_rect.x + es->padding[0] + monstro_font_measure_text(font, col_buf, 0.0f).x;

         MonstroVec2 c1 = {cx, cy};
         MonstroVec2 c2 = {cx, cy + text_h};
         canvas->addLine(c1, c2, monstro_color_to_u32(es->cursor_normal),
                         es->cursor_size > 0.0f ? es->cursor_size : 1.0f);
      }
   }

   canvas->popClipRect();

   if (font && display[0] != '\0')
   {
      MonstroRect label_rect = {pos.x, pos.y, label_w, field_h};
      canvas->addTextEx(font, label_rect, monstro_color_to_u32(style->text.color), display, MONSTRO_ALIGN_DEFAULT);
   }

   monstro_add_item(ctx, total_w, field_h);

   MonstroRect widget_rect = {pos.x, pos.y, total_w, field_h};
   monstro_draw_disabled_overlay(ctx, widget_rect);
   monstro_set_last_item(ctx, id, widget_rect, hovered, is_active, false, value_changed);

   if (hovered && tooltip)
   {
      monstro_tooltip(ctx, tooltip);
   }

   return value_changed;
}

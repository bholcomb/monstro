#include "../context.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

// Width of the first `bytes` bytes of the field's text as drawn; a password
// shows one '*' per code point.
static float text_prefix_width(MonstroFont *font, const char *buf, int bytes, bool password)
{
   if (!font || bytes <= 0)
      return 0.0f;
   if (password)
      return monstro_font_measure_text(font, "*", 0.0f).x * (float)monstro_utf8_count(buf, bytes);
   return monstro_font_measure_prefix(font, buf, (size_t)bytes);
}

static void make_password_text(char *out, size_t out_size, const char *buf)
{
   size_t stars = (size_t)monstro_utf8_count(buf, (int)strlen(buf));
   if (stars > out_size - 1)
      stars = out_size - 1;
   memset(out, '*', stars);
   out[stars] = '\0';
}

static bool is_ctrl_pressed(MonstroContext *ctx) { return (ctx->keyboard.getMods() & MONSTRO_MOD_CTRL) != 0; }

static bool is_shift_pressed(MonstroContext *ctx) { return (ctx->keyboard.getMods() & MONSTRO_MOD_SHIFT) != 0; }

static bool key_pressed(MonstroContext *ctx, MonstroKey key) { return monstro_is_key_pressed(ctx, key, true); }

// The code point boundary nearest to a click.
static int find_click_position(MonstroFont *font, const char *buf, bool password, float click_x, float field_x,
                               float pad_x)
{
   float offset = click_x - field_x - pad_x;
   if (offset <= 0.0f)
      return 0;

   int pos = 0;
   float width = 0.0f;
   while (buf[pos])
   {
      int next = monstro_utf8_next(buf, pos);
      float next_width = text_prefix_width(font, buf, next, password);
      if (next_width >= offset)
         return (offset - width < next_width - offset) ? pos : next;
      pos = next;
      width = next_width;
   }
   return pos;
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

   // The caller may have changed the text since the last frame.
   edit->myCursorPos = monstro_utf8_snap(buf, edit->myCursorPos);
   edit->mySelectStart = monstro_utf8_snap(buf, edit->mySelectStart);
   edit->mySelectEnd = monstro_utf8_snap(buf, edit->mySelectEnd);

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

   char password_text[1024];
   const char *field_text = buf;

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
         edit->activate(id, buf, auto_select);
         is_active = true;
         ctx->want_text_input = true;
      }
      if (is_active && font)
      {
         int click_pos = find_click_position(font, buf, is_password, ctx->mouse.pos_x, field_rect.x, es->padding[0]);
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
                  edit->insertText(buf, buf_size, input);
                  value_changed = true;
                  edit->myCursorBlinkTimer = 0.0f;
               }
            }

            if (key_pressed(ctx, MONSTRO_KEY_BACKSPACE))
            {
               if (edit->myHasSelection)
               {
                  edit->deleteSelection(buf);
                  value_changed = true;
               }
               else if (edit->myCursorPos > 0)
               {
                  text_len = (int)strlen(buf);
                  int prev = monstro_utf8_prev(buf, edit->myCursorPos);
                  memmove(buf + prev, buf + edit->myCursorPos, text_len - edit->myCursorPos + 1);
                  edit->myCursorPos = prev;
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
                  edit->deleteSelection(buf);
                  value_changed = true;
               }
               else
               {
                  text_len = (int)strlen(buf);
                  if (edit->myCursorPos < text_len)
                  {
                     int next = monstro_utf8_next(buf, edit->myCursorPos);
                     memmove(buf + edit->myCursorPos, buf + next, text_len - next + 1);
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
               edit->myCursorPos = monstro_utf8_prev(buf, edit->myCursorPos);
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
               edit->myCursorPos = monstro_utf8_next(buf, edit->myCursorPos);
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
               edit->selectAll(buf);
               edit->myCursorBlinkTimer = 0.0f;
            }
         }
      }
   }

   if (is_password)
   {
      make_password_text(password_text, sizeof(password_text), buf);
      field_text = password_text;
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
      float cursor_x = text_prefix_width(font, buf, edit->myCursorPos, is_password);
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
      float sel_start_x = text_prefix_width(font, buf, sel_s, is_password) - scroll_offset;
      float sel_end_x = text_prefix_width(font, buf, sel_e, is_password) - scroll_offset;
      MonstroRect sel_rect = {text_x + sel_start_x, field_rect.y + 1.0f, sel_end_x - sel_start_x, field_rect.h - 2.0f};
      canvas->addRectFilled(sel_rect, monstro_color_to_u32(es->selected_normal), 0.0f, MONSTRO_CORNER_ALL);
   }

   // Text
   if (font && field_text[0] != '\0')
   {
      MonstroColor text_col = is_active ? es->text_active : (hovered ? es->text_hover : es->text_normal);
      MonstroVec2 text_pos = {text_x - scroll_offset, text_y};
      canvas->addText(font, text_pos, monstro_color_to_u32(text_col), field_text);
   }

   // Cursor
   if (is_active && font)
   {
      bool blink_on = fmodf(edit->myCursorBlinkTimer, 1.0f) < 0.5f;
      if (blink_on)
      {
         float cursor_x_pos = text_prefix_width(font, buf, edit->myCursorPos, is_password) - scroll_offset;
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
         edit->activate(id, buf, false);
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
               edit->insertText(buf, buf_size, "\n");
               value_changed = true;
               edit->myCursorBlinkTimer = 0.0f;
            }

            const char *input = ctx->keyboard.getInputText();
            if (input && input[0] != '\0')
            {
               edit->insertText(buf, buf_size, input);
               value_changed = true;
               edit->myCursorBlinkTimer = 0.0f;
            }

            if (key_pressed(ctx, MONSTRO_KEY_BACKSPACE))
            {
               if (edit->myHasSelection)
               {
                  edit->deleteSelection(buf);
               }
               else if (edit->myCursorPos > 0)
               {
                  text_len = (int)strlen(buf);
                  int prev = monstro_utf8_prev(buf, edit->myCursorPos);
                  memmove(buf + prev, buf + edit->myCursorPos, text_len - edit->myCursorPos + 1);
                  edit->myCursorPos = prev;
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
                  edit->deleteSelection(buf);
               }
               else
               {
                  text_len = (int)strlen(buf);
                  if (edit->myCursorPos < text_len)
                  {
                     int next = monstro_utf8_next(buf, edit->myCursorPos);
                     memmove(buf + edit->myCursorPos, buf + next, text_len - next + 1);
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
               edit->myCursorPos = monstro_utf8_prev(buf, edit->myCursorPos);
               edit->mySelectStart = edit->myCursorPos;
               edit->mySelectEnd = edit->myCursorPos;
               edit->myHasSelection = false;
               edit->myCursorBlinkTimer = 0.0f;
            }
            if (key_pressed(ctx, MONSTRO_KEY_RIGHT_ARROW))
            {
               text_len = (int)strlen(buf);
               edit->myCursorPos = monstro_utf8_next(buf, edit->myCursorPos);
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
               edit->selectAll(buf);
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
         int copy_len =
             line_len < (int)sizeof(line_buf) - 1 ? line_len : (int)monstro_utf8_fit(line_start, sizeof(line_buf) - 1);
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

         const char *ls = buf;
         for (int l = 0; l < cur_line; l++)
         {
            while (*ls && *ls != '\n')
               ls++;
            if (*ls == '\n')
               ls++;
         }
         float cx = field_rect.x + es->padding[0] + monstro_font_measure_prefix(font, ls, (size_t)col);

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

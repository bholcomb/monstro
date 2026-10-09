// UTF-8 decoding, truncation and editing work in whole code points.
#include "context.h"
#include "harness.h"
#include "helpers.h"
#include <cstdlib>
#include <cstring>
#include <string>

// A heap copy with no slack after the terminator, so the sanitizer sees any overread.
static char *exact_copy(const char *text)
{
   size_t size = std::strlen(text) + 1;
   char *copy = (char *)std::malloc(size);
   std::memcpy(copy, text, size);
   return copy;
}

static std::u32string decode_all(const char *text)
{
   char *copy = exact_copy(text);
   std::u32string out;
   const char *s = copy;
   while (*s)
      out.push_back(decode_utf8(&s));
   std::free(copy);
   return out;
}

static bool is_whole_utf8(const char *text)
{
   for (char32_t c : decode_all(text))
      if (c == MONSTRO_REPLACEMENT_CHARACTER)
         return false;
   return true;
}

template <typename Draw> static void frame(Ui &ui, Draw draw)
{
   ui.begin();
   draw();
   ui.end();
}

template <typename Draw> static void key(Ui &ui, MonstroKey which, Draw draw)
{
   ui.pressKey(which, true);
   frame(ui, draw);
   ui.pressKey(which, false);
   frame(ui, draw);
}

template <typename Draw> static void click(Ui &ui, float x, float y, Draw draw)
{
   ui.moveMouse(x, y);
   ui.pressLeft(true);
   frame(ui, draw);
   ui.pressLeft(false);
   frame(ui, draw);
}

TEST(decoder_reads_valid_sequences)
{
   CHECK(decode_all("aé") == U"aé");
   CHECK(decode_all("λ€🚀") == U"λ€🚀");
}

TEST(decoder_stops_at_the_terminator_inside_truncated_sequences)
{
   CHECK(decode_all("\xC3") == U"�");
   CHECK(decode_all("\xE2\x82") == U"�");
   CHECK(decode_all("\xF0\x9F\x9A") == U"�");
   CHECK(decode_all("a\xE2") == U"a�");
}

TEST(decoder_replaces_invalid_sequences)
{
   CHECK(decode_all("\x80") == U"�");
   CHECK(decode_all("\xFF") == U"�");
   CHECK(decode_all("\xC0\xAF") == U"��");
   CHECK(decode_all("\xE0\x80\xAF") == U"�");
   CHECK(decode_all("\xED\xA0\x80") == U"�");
   CHECK(decode_all("\xF4\x90\x80\x80") == U"�");
   CHECK(decode_all("\xE2\x82x") == U"�x");
}

TEST(measuring_and_drawing_truncated_text_stays_inside_the_string)
{
   Ui ui;
   char *text = exact_copy("ab\xE2\x82");
   MonstroVec2 size = monstro_font_measure_text(ui.font, text, 0.0f);
   CHECK(size.x > 0.0f);
   ui.begin();
   monstro_label(ui.ctx, text);
   monstro_label_wrapped(ui.ctx, text, 40.0f);
   ui.end();
   std::free(text);
}

TEST(truncation_keeps_whole_code_points)
{
   char out[4];
   monstro_utf8_copy(out, sizeof(out), "aé€");
   CHECK(std::strcmp(out, "aé") == 0);
   monstro_utf8_copy(out, sizeof(out), "€x");
   CHECK(std::strcmp(out, "€") == 0);
   monstro_utf8_copy(out, sizeof(out), "🚀");
   CHECK(out[0] == '\0');
   CHECK(monstro_utf8_fit("aé", 2) == 1);

   char cut[] = "ab\xE2\x82";
   monstro_utf8_trim_partial(cut);
   CHECK(std::strcmp(cut, "ab") == 0);
   char whole[] = "a€";
   monstro_utf8_trim_partial(whole);
   CHECK(std::strcmp(whole, "a€") == 0);
}

TEST(long_window_titles_and_ids_are_cut_between_code_points)
{
   Ui ui;
   std::string title;
   for (int i = 0; i < 200; i++)
      title += "é";
   ui.begin();
   monstro_begin_window(ui.ctx, title.c_str(), nullptr, MONSTRO_WINDOW_DEFAULT);
   monstro::Window *window = ui.ctx->current_window;
   CHECK(is_whole_utf8(window->title));
   CHECK(is_whole_utf8(window->name));
   CHECK(std::strlen(window->title) == 254);
   monstro_set_window_title(ui.ctx, ("x" + title).c_str());
   CHECK(is_whole_utf8(window->title));
   CHECK(std::strlen(window->title) == 255);
   monstro_end_window(ui.ctx);
   ui.end();
}

TEST(long_table_column_labels_are_cut_between_code_points)
{
   Ui ui;
   std::string label = "x";
   for (int i = 0; i < 100; i++)
      label += "é";
   ui.begin();
   if (monstro_begin_table(ui.ctx, "t", 1, MONSTRO_TABLE_NONE, 0, 0))
   {
      monstro_table_setup_column(ui.ctx, label.c_str(), MONSTRO_TABLE_COLUMN_NONE, 0.0f);
      CHECK(is_whole_utf8(ui.ctx->canvas->table.columns[0].label));
      CHECK(std::strlen(ui.ctx->canvas->table.columns[0].label) == 127);
      monstro_end_table(ui.ctx);
   }
   ui.end();
}

struct TextField
{
   Ui ui;
   char buffer[64] = {};
   size_t size = sizeof(buffer);
   bool multiline = false;
   MonstroInputTextFlags flags = MONSTRO_INPUT_TEXT_NONE;
   MonstroRect rect = {};

   void draw()
   {
      if (multiline)
         monstro_input_text_multiline(ui.ctx, "Field", buffer, size, 300.0f, 100.0f, flags, nullptr);
      else
         monstro_input_text(ui.ctx, "Field", buffer, size, flags, nullptr);
      rect = monstro_get_item_rect(ui.ctx);
   }

   void activate(const char *text)
   {
      std::strcpy(buffer, text);
      frame(ui, [&]() { draw(); });
      click(ui, rect.x + rect.w - 4.0f, rect.y + 8.0f, [&]() { draw(); });
      press(MONSTRO_KEY_END);
   }

   void press(MonstroKey which)
   {
      key(ui, which, [&]() { draw(); });
   }

   void type(const char *text)
   {
      ui.type(text);
      frame(ui, [&]() { draw(); });
   }
};

TEST(backspace_and_delete_remove_whole_code_points)
{
   TextField field;
   field.activate("aé🚀");
   field.press(MONSTRO_KEY_BACKSPACE);
   CHECK(std::strcmp(field.buffer, "aé") == 0);
   field.press(MONSTRO_KEY_BACKSPACE);
   CHECK(std::strcmp(field.buffer, "a") == 0);
   field.type("€b");
   field.press(MONSTRO_KEY_HOME);
   field.press(MONSTRO_KEY_RIGHT_ARROW);
   field.press(MONSTRO_KEY_DELETE);
   CHECK(std::strcmp(field.buffer, "ab") == 0);
}

TEST(arrow_keys_step_over_whole_code_points)
{
   TextField field;
   field.activate("aé");
   field.press(MONSTRO_KEY_LEFT_ARROW);
   field.type("x");
   CHECK(std::strcmp(field.buffer, "axé") == 0);
   field.press(MONSTRO_KEY_RIGHT_ARROW);
   field.type("y");
   CHECK(std::strcmp(field.buffer, "axéy") == 0);
}

TEST(multiline_editing_uses_whole_code_points)
{
   TextField field;
   field.multiline = true;
   field.activate("λ\n€");
   field.press(MONSTRO_KEY_BACKSPACE);
   CHECK(std::strcmp(field.buffer, "λ\n") == 0);
   field.press(MONSTRO_KEY_LEFT_ARROW);
   field.press(MONSTRO_KEY_LEFT_ARROW);
   field.press(MONSTRO_KEY_DELETE);
   CHECK(std::strcmp(field.buffer, "\n") == 0);
}

TEST(typing_into_a_full_buffer_inserts_whole_code_points)
{
   TextField field;
   field.size = 5;
   field.activate("a");
   field.type("é€");
   CHECK(std::strcmp(field.buffer, "aé") == 0);
   CHECK(is_whole_utf8(field.buffer));
}

TEST(password_fields_edit_multibyte_text)
{
   TextField field;
   field.flags = MONSTRO_INPUT_TEXT_PASSWORD;
   field.activate("pä");
   field.type("ß");
   CHECK(std::strcmp(field.buffer, "päß") == 0);
   field.press(MONSTRO_KEY_BACKSPACE);
   CHECK(std::strcmp(field.buffer, "pä") == 0);
}

TEST(a_caller_edit_between_frames_keeps_the_cursor_on_a_boundary)
{
   TextField field;
   field.activate("ab");
   std::strcpy(field.buffer, "é");
   field.type("x");
   CHECK(is_whole_utf8(field.buffer));
}

FATAL_TEST(input_text_overflow_aborts, "input text overflow: capacity 255 bytes per frame, needed 300")
{
   Ui ui(false);
   std::string text(300, 'a');
   monstro_add_input_text(ui.ctx, text.c_str());
}

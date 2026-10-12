// Where widgets put their parts: labels, values and fields stay in the room they are given.
#include "harness.h"
#include <cmath>

static bool near(float a, float b, float tolerance = 0.01f) { return std::fabs(a - b) < tolerance; }

// One fixed window filling the display, and a 1 x 1 rect whose texcoords show where solid shapes sample the atlas:
// the rect's first vertex is returned.
static uint32_t begin_page(Ui &ui)
{
   ui.begin();
   monstro_set_next_window_pos(ui.ctx, 0, 0, MONSTRO_COND_ALWAYS);
   monstro_set_next_window_size(ui.ctx, 800, 600, MONSTRO_COND_ALWAYS);
   monstro_begin_window(ui.ctx, "page", nullptr,
                        (MonstroWindowFlags)(MONSTRO_WINDOW_BACKGROUND | MONSTRO_WINDOW_INPUTS));
   uint32_t first = ui.vertexCount();
   monstro_draw_rect_filled(monstro_get_window_canvas(ui.ctx), {0, 0, 1, 1}, 0xffffffff, 0, MONSTRO_CORNER_NONE);
   return first;
}

struct Bounds
{
   float x0 = 1e9f, y0 = 1e9f, x1 = -1e9f, y1 = -1e9f;
   bool empty() const { return x0 > x1; }
   void add(float x, float y)
   {
      x0 = std::fmin(x0, x), y0 = std::fmin(y0, y), x1 = std::fmax(x1, x), y1 = std::fmax(y1, y);
   }
};

// The bounds of the glyphs drawn in vertices first..last (the vertices that sample the atlas away from its white
// texel), only those whose quads start within x0..x1.
static Bounds glyph_bounds(const MonstroDrawData *data, uint32_t white, uint32_t first, uint32_t last, float x0 = -1e9f,
                           float x1 = 1e9f)
{
   const MonstroVertex &solid = data->vertices[white];
   Bounds b;
   for (uint32_t i = first; i < last; i++)
   {
      const MonstroVertex &v = data->vertices[i];
      if (v.texcoord[0] == solid.texcoord[0] && v.texcoord[1] == solid.texcoord[1])
         continue;
      if (v.position[0] >= x0 && v.position[0] < x1)
         b.add(v.position[0], v.position[1]);
   }
   return b;
}

// The bounds of the vertices first..last drawn in color.
static Bounds colored_bounds(const MonstroDrawData *data, uint32_t first, uint32_t last, uint32_t color)
{
   Bounds b;
   for (uint32_t i = first; i < last; i++)
      if (data->vertices[i].color == color)
         b.add(data->vertices[i].position[0], data->vertices[i].position[1]);
   return b;
}

TEST(a_knob_shows_its_label_and_value_beside_it)
{
   Ui ui;
   float gain = 0.47f;
   uint32_t white = begin_page(ui);
   monstro_set_cursor_screen_pos(ui.ctx, 100, 100);
   uint32_t first = ui.vertexCount();
   monstro_knob(ui.ctx, "Gain", &gain, 0.0f, 1.0f, "%.2f", nullptr);
   uint32_t last = ui.vertexCount();
   MonstroRect item = monstro_get_item_rect(ui.ctx);
   monstro_end_window(ui.ctx);
   const MonstroDrawData *data = ui.end();

   // The knob is its track and body; no glyph lies over it.
   const MonstroStyleKnob *ks = monstro_get_knob_style(ui.ctx);
   Bounds knob = colored_bounds(data, first, last, monstro_color_to_u32(ks->track_normal));
   Bounds body = colored_bounds(data, first, last, monstro_color_to_u32(ks->normal.data.color));
   CHECK(!knob.empty() && !body.empty() && body.x0 > knob.x0 && body.x1 < knob.x1);
   Bounds left = glyph_bounds(data, white, first, last, -1e9f, knob.x0);
   Bounds over = glyph_bounds(data, white, first, last, knob.x0, knob.x1);
   Bounds right = glyph_bounds(data, white, first, last, knob.x1, 1e9f);
   CHECK(over.empty());
   // The label to the left and the value to the right, both within the item.
   CHECK(!left.empty() && left.x1 <= knob.x0 && left.x0 >= item.x);
   CHECK(!right.empty() && right.x0 >= knob.x1 && right.x1 <= item.x + item.w + 1.0f);
   // Both level with the knob's middle.
   float middle = (knob.y0 + knob.y1) * 0.5f;
   CHECK(near((left.y0 + left.y1) * 0.5f, middle, 3.0f) && near((right.y0 + right.y1) * 0.5f, middle, 3.0f));
}

TEST(a_knob_keeps_its_width_as_its_value_changes)
{
   Ui ui;
   float widths[2] = {};
   float values[2] = {0.0f, 100.0f}; // "0.0" and "100.0"
   for (int i = 0; i < 2; i++)
   {
      begin_page(ui);
      monstro_knob(ui.ctx, "Rate", &values[i], 0.0f, 100.0f, "%.1f", nullptr);
      widths[i] = monstro_get_item_rect(ui.ctx).w;
      monstro_end_window(ui.ctx);
      ui.end();
   }
   CHECK(widths[0] == widths[1]);
}

TEST(a_table_cell_holds_the_fields_that_fill_it)
{
   Ui ui;
   char name[32] = "Alpha";
   float value = 1.0f;
   const float table_w = 600.0f, action_w = 80.0f;
   float field_x[3] = {}, field_right[3] = {};
   Bounds value_text;
   const MonstroRect table = {50, 50, table_w, 0}; // the table takes the cursor's x and the width it is given
   MonstroRect after = {};
   for (int frame = 0; frame < 2; frame++)
   {
      uint32_t white = begin_page(ui);
      monstro_set_cursor_screen_pos(ui.ctx, 50, 50);
      CHECK(monstro_begin_table(ui.ctx, "t", 3, MONSTRO_TABLE_BORDERS, table_w, 0.0f));
      monstro_table_setup_column(ui.ctx, "Name", MONSTRO_TABLE_COLUMN_NONE, 0.0f);
      monstro_table_setup_column(ui.ctx, "Value", MONSTRO_TABLE_COLUMN_NONE, 0.0f);
      monstro_table_setup_column(ui.ctx, "Action", MONSTRO_TABLE_COLUMN_WIDTH_FIXED, action_w);
      monstro_table_headers_row(ui.ctx);
      monstro_table_next_row(ui.ctx);
      monstro_table_next_column(ui.ctx);
      monstro_input_text(ui.ctx, "name##", name, sizeof(name), MONSTRO_INPUT_TEXT_NONE, nullptr);
      MonstroRect r = monstro_get_item_rect(ui.ctx);
      field_x[0] = r.x, field_right[0] = r.x + r.w;
      monstro_table_next_column(ui.ctx);
      uint32_t first = ui.vertexCount();
      monstro_input_float(ui.ctx, "value##", &value, 0.0f, 0.0f, "%.2f", nullptr);
      uint32_t last = ui.vertexCount();
      r = monstro_get_item_rect(ui.ctx);
      field_x[1] = r.x, field_right[1] = r.x + r.w;
      monstro_table_set_column_index(ui.ctx, 2);
      monstro_button(ui.ctx, "Reset", nullptr);
      r = monstro_get_item_rect(ui.ctx);
      field_x[2] = r.x, field_right[2] = r.x + r.w;
      monstro_end_table(ui.ctx);
      // After the table the layout has its whole width again.
      monstro_input_text(ui.ctx, "after##", name, sizeof(name), MONSTRO_INPUT_TEXT_NONE, nullptr);
      after = monstro_get_item_rect(ui.ctx);
      monstro_end_window(ui.ctx);
      const MonstroDrawData *data = ui.end();
      value_text = glyph_bounds(data, white, first, last);
   }

   // Each field lies within its column; the value's text within the value column, left of the action column.
   float column_w = (table_w - action_w) * 0.5f;
   float column_x[3] = {table.x, table.x + column_w, table.x + column_w * 2.0f};
   float column_right[3] = {column_x[1], column_x[2], table.x + table_w};
   for (int c = 0; c < 3; c++)
      CHECK(field_x[c] >= column_x[c] && field_right[c] <= column_right[c]);
   // The fields fill their cells.
   CHECK(field_right[0] > column_right[0] - 10.0f && field_right[1] > column_right[1] - 10.0f);
   CHECK(!value_text.empty() && value_text.x0 >= column_x[1] && value_text.x1 <= column_right[1]);
   CHECK(after.w > table_w);
}

// Environments and contexts: shared fonts and themes, lent canvases, draw data, host buffers.
#include "context.h"
#include "env.h"
#include "harness.h"
#include <cstdio>
#include <cstring>
#include <sys/mman.h>
#include <unistd.h>

static MonstroDrawData *frame_copy(Ui &ui, void (*draw)(MonstroContext *))
{
   ui.begin();
   draw(ui.ctx);
   return monstro_draw_data_copy(ui.end());
}

static void draw_panel(MonstroContext *ctx)
{
   monstro_set_next_window_pos(ctx, 40, 40, MONSTRO_COND_ALWAYS);
   monstro_set_next_window_size(ctx, 300, 200, MONSTRO_COND_ALWAYS);
   monstro_begin_window(ctx, "Panel", nullptr, MONSTRO_WINDOW_DEFAULT);
   monstro_label(ctx, "Thrust");
   monstro_button(ctx, "Launch", nullptr);
   MonstroCanvas *canvas = monstro_get_window_canvas(ctx);
   monstro_draw_circle_filled(canvas, {100, 150}, 20, 0xff00ff00, 12);
   monstro_end_window(ctx);
}

static bool same_command(const MonstroDrawCommand &a, const MonstroDrawCommand &b)
{
   return std::memcmp(a.clip_rect, b.clip_rect, sizeof(a.clip_rect)) == 0 && a.texture_id == b.texture_id &&
          a.vertex_offset == b.vertex_offset && a.vertex_count == b.vertex_count && a.index_offset == b.index_offset &&
          a.index_count == b.index_count && a.layer == b.layer && a.custom_draw == b.custom_draw &&
          a.user_data == b.user_data;
}

// Equal commands, indices and content hash; the hash covers every vertex value written.
static bool same_draw_data(const MonstroDrawData *a, const MonstroDrawData *b)
{
   bool same = a->vertex_count == b->vertex_count && a->index_count == b->index_count &&
               a->command_count == b->command_count && a->content_hash == b->content_hash &&
               std::memcmp(a->indices, b->indices, sizeof(uint32_t) * a->index_count) == 0;
   for (uint32_t i = 0; same && i < a->command_count; i++)
      same = same_command(a->commands[i], b->commands[i]);
   return same;
}

/* ------------------------------------------------------------------------- */
/* Sizes                                                                     */
/* ------------------------------------------------------------------------- */

TEST(contexts_are_small)
{
   Ui ui(true);
   size_t context_bytes = ui.ctx->residentBytes();
   size_t env_bytes = ui.env->residentBytes();
   std::printf("  sizeof(Context) %zu bytes, a default context holds %zu bytes; sizeof(Env) %zu, a default "
               "environment holds %zu bytes (canvas %zu, atlas %zu)\n",
               sizeof(monstro::Context), context_bytes, sizeof(monstro::Env), env_bytes,
               ui.env->canvases[0].residentBytes(), (size_t)ui.env->atlas.width * ui.env->atlas.height * 4);
   CHECK(sizeof(monstro::Context) < 8 * 1024);
   CHECK(context_bytes < 64 * 1024);
}

/* ------------------------------------------------------------------------- */
/* Shared fonts and themes                                                   */
/* ------------------------------------------------------------------------- */

TEST(contexts_on_one_env_share_one_font_atlas)
{
   MonstroEnv *env = monstro_init(nullptr);
   MonstroFont *font = load_test_font(env);
   monstro_env_set_atlas_texture(env, 7);
   uint64_t version = monstro_env_get_atlas(env).version;
   {
      Ui a(env), b(env);
      CHECK(a.font == font && b.font == font);
      a.begin();
      monstro_label(a.ctx, "shared glyphs");
      const MonstroDrawData *da = a.end();
      MonstroDrawData *copy_a = monstro_draw_data_copy(da);
      b.begin();
      monstro_label(b.ctx, "shared glyphs");
      const MonstroDrawData *db = b.end();
      // The same text in two contexts samples the same atlas texels.
      CHECK(same_draw_data(copy_a, db));
      bool all_atlas = true;
      for (uint32_t i = 0; i < db->command_count; i++)
         all_atlas = all_atlas && db->commands[i].texture_id == 7;
      CHECK(all_atlas);
      monstro_draw_data_free(copy_a);
   }
   CHECK(monstro_env_get_atlas(env).version == version);
   CHECK(monstro_font_find(env, "test", 16.0f) == font);
   CHECK(env->fonts.size() == 1);
   monstro_shutdown(env);
}

TEST(atlas_has_a_white_texel_and_tracks_loads)
{
   MonstroEnv *env = monstro_init(nullptr);
   MonstroTextureData atlas = monstro_env_get_atlas(env);
   CHECK(atlas.format == MONSTRO_TEXTURE_FORMAT_RGBA8);
   CHECK(atlas.data_size == (size_t)atlas.width * atlas.height * 4);
   MonstroVec2 white = env->atlas.white_uv;
   uint32_t x = (uint32_t)(white.x * (float)atlas.width);
   uint32_t y = (uint32_t)(white.y * (float)atlas.height);
   const uint8_t *texel = (const uint8_t *)atlas.data + ((size_t)y * atlas.width + x) * 4;
   CHECK(texel[0] == 255 && texel[1] == 255 && texel[2] == 255 && texel[3] == 255);
   load_test_font(env);
   CHECK(monstro_env_get_atlas(env).version > atlas.version);
   monstro_shutdown(env);
}

TEST(fills_and_text_batch_into_one_command)
{
   Ui ui(true);
   ui.begin();
   MonstroCanvas *canvas = monstro_get_window_canvas(ui.ctx);
   monstro_draw_rect_filled(canvas, {10, 10, 50, 20}, 0xff0000ff, 0, MONSTRO_CORNER_NONE);
   monstro_draw_text(canvas, ui.font, {10, 40}, 0xffffffff, "text");
   monstro_draw_rect_filled(canvas, {10, 70, 50, 20}, 0xff00ff00, 4, MONSTRO_CORNER_ALL);
   const MonstroDrawData *data = ui.end();
   CHECK(data->command_count == 1);
   CHECK(data->commands[0].texture_id == monstro_env_get_atlas_texture(ui.env));
}

TEST(a_theme_is_shared_and_pushes_stay_in_the_frame)
{
   MonstroEnv *env = monstro_init(nullptr);
   MonstroStyle style;
   monstro_style_set_default(&style);
   style.button.rounding = 3.0f;
   MonstroTheme *hud = monstro_theme_register(env, "hud", &style);
   CHECK(monstro_theme_find(env, "hud") == hud);
   CHECK(monstro_theme_find(env, "default") != nullptr);
   {
      Ui a(env), b(env);
      monstro_context_set_theme(a.ctx, hud);
      monstro_context_set_theme(b.ctx, hud);
      CHECK(monstro_get_button_style(a.ctx)->rounding == 3.0f);

      a.begin();
      MonstroStyleButton pushed = *monstro_get_button_style(a.ctx);
      pushed.rounding = 33.0f;
      monstro_push_button_style(a.ctx, &pushed);
      CHECK(monstro_get_button_style(a.ctx)->rounding == 33.0f);
      CHECK(monstro_theme_get_style(hud)->button.rounding == 3.0f);
      monstro_pop_style(a.ctx, 1);
      a.end();

      b.begin();
      CHECK(monstro_get_button_style(b.ctx)->rounding == 3.0f);
      b.end();
      a.begin();
      CHECK(monstro_get_button_style(a.ctx)->rounding == 3.0f);
      a.end();
   }
   monstro_shutdown(env);
}

FATAL_TEST(themes_are_registered_once, "a theme named 'dark' is already registered")
{
   MonstroEnv *env = monstro_init(nullptr);
   MonstroStyle style;
   monstro_style_set_dark(&style);
   monstro_theme_register(env, "dark", &style);
}

FATAL_TEST(the_env_is_read_only_during_frames, "monstro_font_load_ttf_from_file: the environment is read-only")
{
   Ui ui(false);
   ui.begin();
   load_test_font(ui.env);
}

FATAL_TEST(shutdown_with_live_contexts_aborts, "monstro_shutdown: 1 contexts still use this environment")
{
   MonstroEnv *env = monstro_init(nullptr);
   monstro_context_create(env, nullptr);
   monstro_shutdown(env);
}

FATAL_TEST(font_atlas_overflow_aborts, "font atlas full: capacity 64 x 64 pixels", "for test",
           "raise MonstroEnvConfig.atlas_width and atlas_height")
{
   MonstroEnvConfig config = monstro_env_config_default();
   config.atlas_width = 64;
   config.atlas_height = 64;
   load_test_font(monstro_init(&config));
}

FATAL_TEST(font_table_overflow_aborts, "font table full: capacity 1, needed 2", "raise MonstroEnvConfig.max_fonts")
{
   MonstroEnvConfig config = monstro_env_config_default();
   config.max_fonts = 1;
   MonstroEnv *env = monstro_init(&config);
   load_test_font(env);
   load_test_font(env);
}

TEST(a_missing_font_file_returns_null)
{
   MonstroEnv *env = monstro_init(nullptr);
   CHECK(monstro_font_load_ttf_from_file(env, "missing", "no/such/font.ttf", 16.0f, 32, 96) == nullptr);
   CHECK(monstro_env_get_default_font(env) == nullptr);
   monstro_shutdown(env);
}

/* ------------------------------------------------------------------------- */
/* Lending the canvas                                                        */
/* ------------------------------------------------------------------------- */

TEST(one_canvas_is_lent_to_each_context_in_turn)
{
   MonstroEnv *env = monstro_init(nullptr);
   {
      Ui a(env), b(env), c(env);
      a.begin();
      monstro_draw_rect_filled(monstro_get_window_canvas(a.ctx), {0, 0, 10, 10}, 0xffffffff, 0, MONSTRO_CORNER_NONE);
      const MonstroDrawData *da = a.end();
      MonstroDrawData *kept = monstro_draw_data_copy(da);
      CHECK(da->vertex_count == 4);

      uint64_t a_frame = da->frame_id;
      b.begin();
      const MonstroDrawData *db = b.end();
      // One canvas: b's frame reused it, so a's draw data now shows b's frame. The copy kept a's.
      CHECK(db == da);
      CHECK(da->frame_id == a_frame + 1 && kept->frame_id == a_frame);
      CHECK(da->vertex_count == 0);
      CHECK(kept->vertex_count == 4 && kept->vertices[2].position[0] == 10.0f);

      c.begin();
      CHECK(c.ctx->canvas == a.ctx->env->canvases);
      c.end();
      monstro_draw_data_free(kept);
   }
   monstro_shutdown(env);
}

TEST(with_two_canvases_the_previous_frame_stays_valid)
{
   MonstroEnvConfig config = monstro_env_config_default();
   config.canvas_count = 2;
   MonstroEnv *env = monstro_init(&config);
   {
      Ui a(env), b(env);
      a.begin();
      monstro_draw_rect_filled(monstro_get_window_canvas(a.ctx), {0, 0, 10, 10}, 0xffffffff, 0, MONSTRO_CORNER_NONE);
      const MonstroDrawData *da = a.end();
      MonstroDrawData *kept = monstro_draw_data_copy(da);
      b.begin();
      const MonstroDrawData *db = b.end();
      CHECK(db != da);
      CHECK(same_draw_data(da, kept));

      // Frames of two contexts may be open at once, one canvas each.
      a.begin();
      b.begin();
      b.end();
      a.end();
      monstro_draw_data_free(kept);
   }
   monstro_shutdown(env);
}

FATAL_TEST(all_canvases_lent_aborts, "all 1 canvases are lent: capacity 1, needed 2 open frames",
           "raise MonstroEnvConfig.canvas_count")
{
   MonstroEnv *env = monstro_init(nullptr);
   Ui a(env), b(env);
   a.begin();
   b.begin();
}

FATAL_TEST(drawing_on_a_returned_canvas_aborts, "monstro_draw_line: the canvas is not lent to a frame")
{
   Ui ui(false);
   ui.begin();
   MonstroCanvas *canvas = monstro_get_window_canvas(ui.ctx);
   ui.end();
   monstro_draw_line(canvas, {0, 0}, {1, 1}, 0xffffffff, 1.0f);
}

TEST(draw_commands_carry_their_vertex_ranges)
{
   Ui ui(true);
   ui.begin();
   draw_panel(ui.ctx);
   monstro_set_next_window_pos(ui.ctx, 200, 100, MONSTRO_COND_ALWAYS);
   monstro_begin_window(ui.ctx, "Other", nullptr, MONSTRO_WINDOW_DEFAULT);
   monstro_label(ui.ctx, "second window");
   monstro_end_window(ui.ctx);
   const MonstroDrawData *data = ui.end();
   CHECK(data->command_count > 1);

   uint32_t vertices_seen = 0, indices_seen = 0;
   bool in_range = true;
   for (uint32_t c = 0; c < data->command_count; c++)
   {
      const MonstroDrawCommand &cmd = data->commands[c];
      in_range = in_range && cmd.vertex_offset + cmd.vertex_count <= data->vertex_count &&
                 cmd.index_offset + cmd.index_count <= data->index_count;
      for (uint32_t i = 0; i < cmd.index_count; i++)
         in_range = in_range && data->indices[cmd.index_offset + i] < cmd.vertex_count;
      vertices_seen += cmd.vertex_count;
      indices_seen += cmd.index_count;
   }
   CHECK(in_range);
   CHECK(vertices_seen == data->vertex_count && indices_seen == data->index_count);
}

TEST(the_content_hash_tells_whether_a_frame_changed)
{
   Ui ui(true);
   MonstroDrawData *first = frame_copy(ui, draw_panel);
   CHECK(first->changed);
   MonstroDrawData *second = frame_copy(ui, draw_panel);
   CHECK(!second->changed && second->content_hash == first->content_hash);
   ui.begin();
   draw_panel(ui.ctx);
   monstro_draw_line(monstro_get_window_canvas(ui.ctx), {0, 0}, {5, 5}, 0xffffffff, 1.0f);
   const MonstroDrawData *third = ui.end();
   CHECK(third->changed && third->content_hash != first->content_hash);
   monstro_draw_data_free(first);
   monstro_draw_data_free(second);
}

TEST(windows_draw_in_front_to_back_order)
{
   Ui ui(false);
   for (int frame = 0; frame < 2; frame++)
   {
      ui.begin();
      monstro_begin_window(ui.ctx, "Back", nullptr, MONSTRO_WINDOW_BACKGROUND);
      monstro_draw_line(monstro_get_window_canvas(ui.ctx), {0, 0}, {1, 1}, 0xff000001, 1.0f);
      monstro_end_window(ui.ctx);
      monstro_begin_window(ui.ctx, "Front", nullptr, MONSTRO_WINDOW_BACKGROUND);
      monstro_draw_line(monstro_get_window_canvas(ui.ctx), {0, 0}, {1, 1}, 0xff000002, 1.0f);
      // Drawing returns to Back after Front: still drawn below Front.
      monstro_end_window(ui.ctx);
      monstro_begin_window(ui.ctx, "Back", nullptr, MONSTRO_WINDOW_BACKGROUND);
      monstro_draw_line(monstro_get_window_canvas(ui.ctx), {2, 2}, {3, 3}, 0xff000003, 1.0f);
      monstro_end_window(ui.ctx);
      ui.end();
   }
   const MonstroDrawData *data = ui.data;
   // The last command drawn is Front's, ending with its line.
   const MonstroDrawCommand &last = data->commands[data->command_count - 1];
   CHECK(data->vertices[last.vertex_offset + last.vertex_count - 1].color == 0xff000002);
}

/* ------------------------------------------------------------------------- */
/* Host buffers                                                              */
/* ------------------------------------------------------------------------- */

// Host memory that ends at a guard page and starts poisoned, so writes past the
// capacity fault and values Monstro read back would show up as poison.
struct HostMemory
{
   uint8_t *block = nullptr;
   size_t block_size = 0;
   uint8_t *data = nullptr;

   explicit HostMemory(size_t bytes)
   {
      size_t page = (size_t)sysconf(_SC_PAGESIZE);
      size_t pages = (bytes + page - 1) / page;
      block_size = (pages + 1) * page;
      block = (uint8_t *)mmap(nullptr, block_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
      mprotect(block + pages * page, page, PROT_NONE);
      data = block + pages * page - bytes;
      std::memset(data, 0xcd, bytes);
   }
   ~HostMemory() { munmap(block, block_size); }
};

static void draw_into(Ui &ui, const MonstroFrameBuffers *buffers)
{
   if (buffers)
      monstro_begin_frame_into(ui.ctx, 1.0f / 60.0f, buffers);
   else
      ui.begin();
   draw_panel(ui.ctx);
   ui.end();
}

TEST(host_buffers_receive_the_same_frame)
{
   const uint32_t capacity = 4096;

   // (a) The canvas's own buffers.
   Ui own(true);
   draw_into(own, nullptr);
   draw_into(own, nullptr);
   MonstroDrawData *expected = monstro_draw_data_copy(own.data);

   // (b) One interleaved host buffer, Monstro's own layout.
   Ui interleaved(true);
   draw_into(interleaved, nullptr);
   HostMemory vertex_memory(capacity * sizeof(MonstroVertex));
   HostMemory index_memory(capacity * 3 * sizeof(uint32_t));
   MonstroFrameBuffers buffers = {};
   buffers.vertex_buffers[0] = vertex_memory.data;
   buffers.vertex_capacity = capacity;
   buffers.indices = (uint32_t *)index_memory.data;
   buffers.index_capacity = capacity * 3;
   draw_into(interleaved, &buffers);
   const MonstroDrawData *got = interleaved.data;
   CHECK(got->vertices == nullptr && got->indices == buffers.indices);
   CHECK(same_draw_data(expected, got));
   CHECK(std::memcmp(vertex_memory.data, expected->vertices, sizeof(MonstroVertex) * expected->vertex_count) == 0);

   // (c) Split streams: positions alone, then texcoord and color together, color first.
   MonstroEnvConfig config = monstro_env_config_default();
   config.vertex_layout.buffer_count = 2;
   config.vertex_layout.stride[0] = 8;
   config.vertex_layout.stride[1] = 12;
   config.vertex_layout.position = {0, 0};
   config.vertex_layout.color = {1, 0};
   config.vertex_layout.texcoord = {1, 4};
   Ui split(true, nullptr, &config);
   draw_into(split, nullptr);
   HostMemory positions(capacity * 8);
   HostMemory attributes(capacity * 12);
   HostMemory split_indices(capacity * 3 * sizeof(uint32_t));
   MonstroFrameBuffers split_buffers = {};
   split_buffers.vertex_buffers[0] = positions.data;
   split_buffers.vertex_buffers[1] = attributes.data;
   split_buffers.vertex_capacity = capacity;
   split_buffers.indices = (uint32_t *)split_indices.data;
   split_buffers.index_capacity = capacity * 3;
   draw_into(split, &split_buffers);
   CHECK(same_draw_data(expected, split.data));
   bool same_vertices = true;
   for (uint32_t i = 0; i < expected->vertex_count; i++)
   {
      const MonstroVertex &v = expected->vertices[i];
      float position[2], texcoord[2];
      uint32_t color;
      std::memcpy(position, positions.data + i * 8, 8);
      std::memcpy(&color, attributes.data + i * 12, 4);
      std::memcpy(texcoord, attributes.data + i * 12 + 4, 8);
      same_vertices = same_vertices && position[0] == v.position[0] && position[1] == v.position[1] &&
                      texcoord[0] == v.texcoord[0] && texcoord[1] == v.texcoord[1] && color == v.color;
   }
   CHECK(same_vertices);
   monstro_draw_data_free(expected);
}

FATAL_TEST(a_missing_host_stream_aborts, "vertex_buffers[1] is null but the environment's vertex layout has 2 buffers")
{
   MonstroEnvConfig config = monstro_env_config_default();
   config.vertex_layout.buffer_count = 2;
   config.vertex_layout.stride[0] = 8;
   config.vertex_layout.stride[1] = 12;
   config.vertex_layout.position = {0, 0};
   config.vertex_layout.color = {1, 0};
   config.vertex_layout.texcoord = {1, 4};
   Ui ui(false, nullptr, &config);
   static float positions[64];
   MonstroFrameBuffers buffers = {};
   buffers.vertex_buffers[0] = positions;
   buffers.vertex_capacity = 32;
   monstro_begin_frame_into(ui.ctx, 0.0f, &buffers);
}

FATAL_TEST(overlapping_attributes_abort, "vertex layout: texcoord (offset 8) and color (offset 12) overlap in buffer 0")
{
   MonstroEnvConfig config = monstro_env_config_default();
   config.vertex_layout.color = {0, 12};
   monstro_init(&config);
}

FATAL_TEST(an_attribute_outside_its_stride_aborts,
           "vertex layout: color (4 bytes at offset 18) does not fit buffer 0's stride of 20 bytes")
{
   MonstroEnvConfig config = monstro_env_config_default();
   config.vertex_layout.color = {0, 18};
   monstro_init(&config);
}

FATAL_TEST(an_attribute_in_a_missing_buffer_aborts,
           "vertex layout: position is in buffer 1 but the layout has 1 buffers")
{
   MonstroEnvConfig config = monstro_env_config_default();
   config.vertex_layout.position = {1, 0};
   monstro_init(&config);
}

FATAL_TEST(an_unused_vertex_buffer_aborts, "vertex layout: buffer 1 holds no attribute")
{
   MonstroEnvConfig config = monstro_env_config_default();
   config.vertex_layout.buffer_count = 2;
   config.vertex_layout.stride[1] = 8;
   monstro_init(&config);
}

/* ------------------------------------------------------------------------- */
/* Capacities from config                                                    */
/* ------------------------------------------------------------------------- */

FATAL_TEST(the_window_capacity_comes_from_the_context_config, "window table full: capacity 4, needed 5",
           "raise MonstroContextConfig.max_windows")
{
   MonstroContextConfig config = monstro_context_config_default();
   config.max_windows = 4;
   Ui ui(false, &config);
   ui.begin();
   const char *names[] = {"A", "B", "C", "D"};
   for (const char *name : names)
   {
      monstro_begin_window(ui.ctx, name, nullptr, MONSTRO_WINDOW_DEFAULT);
      monstro_end_window(ui.ctx);
   }
}

FATAL_TEST(icon_table_overflow_aborts, "icon table full: capacity 4, needed 5", "raise MonstroEnvConfig.max_icons")
{
   MonstroEnvConfig config = monstro_env_config_default();
   config.max_icons = 4;
   MonstroEnv *env = monstro_init(&config);
   monstro_register_icon_atlas(env, 1, 9, 16, 16, 64, 32, 4, 2);
}

TEST(icons_live_in_a_fixed_table)
{
   MonstroEnvConfig config = monstro_env_config_default();
   config.max_icons = 8;
   MonstroEnv *env = monstro_init(&config);
   monstro_register_icon_atlas(env, 100, 9, 16, 16, 64, 32, 4, 2);
   uint64_t texture = 0;
   MonstroVec2 uv_min, uv_max;
   CHECK(monstro_get_icon(env, 105, &texture, &uv_min, &uv_max) && texture == 9);
   CHECK(uv_min.x == 0.25f && uv_min.y == 0.5f && uv_max.x == 0.5f && uv_max.y == 1.0f);
   // Registering an ID again replaces it without taking another slot.
   monstro_register_icon(env, 105, 10, {0, 0}, {1, 1});
   CHECK(monstro_get_icon(env, 105, &texture, nullptr, nullptr) && texture == 10);
   CHECK(!monstro_get_icon(env, 99, nullptr, nullptr, nullptr));
   monstro_shutdown(env);
}

/* ------------------------------------------------------------------------- */
/* Fail hard at the boundary                                                 */
/* ------------------------------------------------------------------------- */

FATAL_TEST(a_null_label_aborts, "monstro_button_ex: label must not be null")
{
   Ui ui(false);
   ui.begin();
   monstro_button(ui.ctx, nullptr, nullptr);
}

FATAL_TEST(a_null_value_aborts, "monstro_checkbox: value must not be null")
{
   Ui ui(false);
   ui.begin();
   monstro_checkbox(ui.ctx, "check", nullptr, nullptr);
}

FATAL_TEST(a_null_context_in_a_getter_aborts, "monstro_is_item_hovered: the context must not be null")
{
   monstro_is_item_hovered(nullptr);
}

FATAL_TEST(a_null_context_in_input_aborts, "monstro_set_mouse_pos: the context must not be null")
{
   monstro_set_mouse_pos(nullptr, 1, 2);
}

FATAL_TEST(a_null_env_aborts, "monstro_font_find: the environment must not be null")
{
   monstro_font_find(nullptr, "test", 0.0f);
}

FATAL_TEST(a_null_canvas_aborts, "monstro_draw_rect_filled: the canvas must not be null")
{
   monstro_draw_rect_filled(nullptr, {0, 0, 1, 1}, 0, 0, MONSTRO_CORNER_NONE);
}

/* ------------------------------------------------------------------------- */
/* Tab bars, popups and child regions                                        */
/* ------------------------------------------------------------------------- */

TEST(tab_bars_nest)
{
   Ui ui(true);
   uint32_t before_b = 0, after_b = 0;
   ui.begin();
   monstro_begin_window(ui.ctx, "Tabs", nullptr, MONSTRO_WINDOW_DEFAULT);
   CHECK(monstro_begin_tab_bar(ui.ctx, "outer", MONSTRO_TAB_BAR_NONE));
   CHECK(monstro_begin_tab_item(ui.ctx, "A", nullptr));
   CHECK(monstro_begin_tab_bar(ui.ctx, "inner", MONSTRO_TAB_BAR_NONE));
   CHECK(monstro_begin_tab_item(ui.ctx, "Inner A", nullptr));
   monstro_end_tab_item(ui.ctx);
   monstro_end_tab_bar(ui.ctx);
   monstro_end_tab_item(ui.ctx);
   // The outer bar is current again: its next tab is drawn into it.
   before_b = ui.vertexCount();
   CHECK(!monstro_begin_tab_item(ui.ctx, "B", nullptr));
   after_b = ui.vertexCount();
   monstro_end_tab_bar(ui.ctx);
   monstro_end_window(ui.ctx);
   ui.end();
   CHECK(after_b > before_b);
}

FATAL_TEST(a_tab_item_outside_a_tab_bar_aborts, "monstro_begin_tab_item outside a tab bar")
{
   Ui ui(false);
   ui.begin();
   monstro_begin_tab_item(ui.ctx, "lost", nullptr);
}

static void popup_frame(Ui &ui, bool open, bool close_outer)
{
   ui.begin();
   monstro_begin_window(ui.ctx, "Owner", nullptr, MONSTRO_WINDOW_DEFAULT);
   if (open)
      monstro_open_popup(ui.ctx, "Outer");
   if (monstro_begin_popup(ui.ctx, "Outer", MONSTRO_WINDOW_NONE))
   {
      if (open)
         monstro_open_popup(ui.ctx, "Inner");
      if (monstro_begin_popup(ui.ctx, "Inner", MONSTRO_WINDOW_NONE))
      {
         monstro_label(ui.ctx, "inner");
         monstro_end_popup(ui.ctx);
      }
      // Closes Outer, the popup being built, and Inner with it: not merely the top of the stack.
      if (close_outer)
         monstro_close_current_popup(ui.ctx);
      monstro_end_popup(ui.ctx);
   }
   monstro_end_window(ui.ctx);
   ui.end();
}

TEST(close_current_popup_closes_the_popup_being_built)
{
   Ui ui(true);
   popup_frame(ui, true, false);
   CHECK(ui.ctx->popup_stack.size() == 2);
   popup_frame(ui, false, true);
   CHECK(ui.ctx->popup_stack.size() == 0);
}

FATAL_TEST(close_current_popup_outside_a_popup_aborts, "monstro_close_current_popup called outside a popup")
{
   Ui ui(false);
   ui.begin();
   monstro_close_current_popup(ui.ctx);
}

TEST(default_windows_are_not_child_regions) { CHECK(!(MONSTRO_WINDOW_DEFAULT & MONSTRO_WINDOW_CHILD)); }

FATAL_TEST(end_child_on_a_window_aborts, "monstro_end_child: the current window 'Plain' was not begun by "
                                         "monstro_begin_child")
{
   Ui ui(false);
   ui.begin();
   monstro_begin_window(ui.ctx, "Plain", nullptr, MONSTRO_WINDOW_DEFAULT);
   monstro_end_child(ui.ctx);
}

FATAL_TEST(end_window_on_a_child_region_aborts, "end it with monstro_end_child")
{
   Ui ui(false);
   ui.begin();
   monstro_begin_child(ui.ctx, "region", 100, 100, MONSTRO_WINDOW_NONE);
   monstro_end_window(ui.ctx);
}

TEST(child_regions_pair_with_end_child)
{
   Ui ui(true);
   for (int frame = 0; frame < 2; frame++)
   {
      ui.begin();
      monstro_begin_window(ui.ctx, "Parent", nullptr, MONSTRO_WINDOW_DEFAULT);
      CHECK(monstro_begin_child(ui.ctx, "region", 100, 60, MONSTRO_WINDOW_CHILD));
      monstro_label(ui.ctx, "inside");
      monstro_end_child(ui.ctx);
      monstro_label(ui.ctx, "after");
      monstro_end_window(ui.ctx);
      ui.end();
   }
}

TEST(pressing_an_open_combo_closes_its_list)
{
   Ui ui(true);
   MonstroRect box = {};
   auto frame = [&](bool open)
   {
      ui.begin();
      monstro_begin_window(ui.ctx, "A", nullptr, MONSTRO_WINDOW_DEFAULT);
      if (open)
         monstro_open_popup(ui.ctx, "Mode");
      bool listed = monstro_begin_combo(ui.ctx, "Mode", "Orbit", nullptr);
      box = monstro_get_item_rect(ui.ctx);
      if (listed)
         monstro_end_combo(ui.ctx);
      monstro_end_window(ui.ctx);
      ui.end();
   };
   frame(true);
   CHECK(ui.ctx->popup_stack.size() == 1);
   // The press lands on the combo box itself, outside any popup.
   ui.moveMouse(box.x + box.w - 10.0f, box.y + box.h * 0.5f);
   ui.pressLeft(true);
   frame(false);
   ui.pressLeft(false);
   frame(false);
   CHECK(ui.ctx->popup_stack.size() == 0);
}

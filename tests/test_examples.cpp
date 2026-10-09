// The example widget libraries run headless under every check, with the mouse
// sweeping, clicking and dragging and text being typed, as a smoke test.
#include "../examples/console/console.h"
#include "../examples/curve/curve.h"
#include "../examples/node/node.h"
#include "../examples/plot/plot.h"
#include "../examples/property/property.h"
#include "../examples/shortcut/shortcut.h"
#include "../examples/timeline/timeline.h"
#include "../examples/toast/toast.h"
#include "harness.h"
#include <cmath>
#include <memory>

template <typename Draw> static void exercise(Draw draw)
{
   Ui ui;
   for (int frame = 0; frame < 240; frame++)
   {
      float x = (float)((frame * 37) % 800);
      float y = (float)((frame * 53) % 600);
      ui.moveMouse(x, y);
      ui.pressLeft(frame % 6 < 3);
      if (frame % 17 == 0)
         ui.type("aé1");
      ui.pressKey(MONSTRO_KEY_BACKSPACE, frame % 23 == 0);
      ui.begin(1.0f / 30.0f);
      draw(ui.ctx);
      ui.end();
   }
}

template <typename Draw> static void in_window(MonstroContext *ctx, const char *name, Draw draw)
{
   monstro_set_next_window_pos(ctx, 50, 50, MONSTRO_COND_FIRST_USE_EVER);
   monstro_set_next_window_size(ctx, 560, 500, MONSTRO_COND_FIRST_USE_EVER);
   if (monstro_begin_window(ctx, name, nullptr, MONSTRO_WINDOW_DEFAULT))
   {
      draw();
      monstro_end_window(ctx);
   }
}

TEST(console_example_runs)
{
   auto console = std::make_unique<Console>();
   console_init(console.get());
   for (int i = 0; i < 50; i++)
      console_log(console.get(), CONSOLE_LOG_INFO, "line %d: ünïcødé", i);
   exercise([&](MonstroContext *ctx) { console_draw(console.get(), ctx, 1.0f / 30.0f); });
}

TEST(curve_example_runs)
{
   auto curve = std::make_unique<CurveEditor>();
   curve_editor_init(curve.get(), 500, 350);
   curve_editor_add_point(curve.get(), 0.0f, 0.0f);
   curve_editor_add_point(curve.get(), 0.5f, 0.3f);
   curve_editor_add_point(curve.get(), 1.0f, 1.0f);
   exercise([&](MonstroContext *ctx)
            { in_window(ctx, "Curve Editor", [&]() { curve_editor_draw(curve.get(), ctx); }); });
}

TEST(node_example_runs)
{
   auto nodes = std::make_unique<NodeEditor>();
   node_editor_init(nodes.get());
   int a = node_editor_add_node(nodes.get(), "Texture", 50, 100, 0xFF446688);
   node_editor_add_port(nodes.get(), a, "Color", true);
   int b = node_editor_add_node(nodes.get(), "Output", 300, 150, 0xFF884466);
   node_editor_add_port(nodes.get(), b, "Color", false);
   node_editor_add_link(nodes.get(), a, 0, b, 0);
   exercise([&](MonstroContext *ctx) { node_editor_draw(nodes.get(), ctx); });
}

TEST(plot_example_runs)
{
   auto plot = std::make_unique<PlotWidget>();
   plot_init(plot.get(), "Sine", 500, 200);
   int series = plot_add_series(plot.get(), "sin(x)", PLOT_LINE, 0xFF4488FF);
   float xs[200], ys[200];
   for (int i = 0; i < 200; i++)
   {
      xs[i] = (float)i * 0.05f;
      ys[i] = std::sin(xs[i]);
   }
   plot_set_data(plot.get(), series, xs, ys, 200);
   exercise([&](MonstroContext *ctx) { in_window(ctx, "Plots", [&]() { plot_draw(plot.get(), ctx); }); });
}

TEST(property_example_runs)
{
   auto editor = std::make_unique<PropertyEditor>();
   property_editor_init(editor.get());
   property_editor_add_section(editor.get(), "Transform");
   property_editor_add_float(editor.get(), "Position X", 0.0f, -100.0f, 100.0f);
   property_editor_add_bool(editor.get(), "Visible", true);
   property_editor_add_color(editor.get(), "Tint", {255, 255, 255, 255});
   const char *modes[] = {"Opaque", "Alpha Blend"};
   property_editor_add_enum(editor.get(), "Blend Mode", 0, modes, 2);
   property_editor_add_string(editor.get(), "Name", "GameObject_01");
   property_editor_add_int(editor.get(), "Layer", 0, 0, 31);
   exercise([&](MonstroContext *ctx) { property_editor_draw(editor.get(), ctx); });
}

TEST(shortcut_example_runs)
{
   auto editor = std::make_unique<ShortcutEditor>();
   shortcut_editor_init(editor.get());
   shortcut_editor_add(editor.get(), "Save", MONSTRO_MOD_CTRL, MONSTRO_KEY_S);
   shortcut_editor_add(editor.get(), "Delete", MONSTRO_MOD_NONE, MONSTRO_KEY_DELETE);
   exercise([&](MonstroContext *ctx) { shortcut_editor_draw(editor.get(), ctx); });
}

TEST(timeline_example_runs)
{
   auto timeline = std::make_unique<Timeline>();
   timeline_init(timeline.get(), 30.0f);
   int track = timeline_add_track(timeline.get(), "Audio");
   timeline_add_clip(timeline.get(), track, "Intro", 0.0f, 5.0f, 0xFF4488AA);
   exercise([&](MonstroContext *ctx) { timeline_draw(timeline.get(), ctx, 1.0f / 30.0f); });
}

TEST(toast_example_runs)
{
   auto toasts = std::make_unique<ToastManager>();
   toast_init(toasts.get());
   exercise(
       [&](MonstroContext *ctx)
       {
          in_window(ctx, "Toast Controls",
                    [&]()
                    {
                       if (monstro_button(ctx, "Info Toast", nullptr))
                          toast_show(toasts.get(), TOAST_INFO, 3.0f, "Information message");
                    });
          toast_draw(toasts.get(), ctx, 1.0f / 30.0f);
       });
}

#include "../common/example_app.h"
#include "plot.h"
#include <cmath>

static PlotWidget g_line_plot;
static PlotWidget g_bar_plot;
static PlotWidget g_scatter_plot;

static float g_time = 0.0f;

static void on_init(ExampleApp *, void *)
{
   plot_init(&g_line_plot, "Sine / Cosine", 500, 200);
   int s0 = plot_add_series(&g_line_plot, "sin(x)", PLOT_LINE, 0xFF4488FF);
   int s1 = plot_add_series(&g_line_plot, "cos(x)", PLOT_LINE, 0xFF44FF88);
   float x[100], y0[100], y1[100];
   for (int i = 0; i < 100; i++)
   {
      x[i] = (float)i * 0.1f;
      y0[i] = sinf(x[i]);
      y1[i] = cosf(x[i]);
   }
   plot_set_data(&g_line_plot, s0, x, y0, 100);
   plot_set_data(&g_line_plot, s1, x, y1, 100);

   plot_init(&g_bar_plot, "Monthly Sales", 500, 200);
   int b0 = plot_add_series(&g_bar_plot, "Revenue", PLOT_BAR, 0xFF8844FF);
   float bx[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
   float by[] = {12, 19, 8, 15, 22, 27, 20, 18, 25, 30, 28, 35};
   plot_set_data(&g_bar_plot, b0, bx, by, 12);

   plot_init(&g_scatter_plot, "Random Points", 500, 200);
   int sc0 = plot_add_series(&g_scatter_plot, "Points", PLOT_SCATTER, 0xFF44CCFF);
   float sx[50], sy[50];
   for (int i = 0; i < 50; i++)
   {
      sx[i] = (float)(i % 10) + (float)(i * 7 % 5) * 0.3f;
      sy[i] = sinf((float)i * 0.5f) * 3.0f + (float)(i * 13 % 7) * 0.5f;
   }
   plot_set_data(&g_scatter_plot, sc0, sx, sy, 50);
}

static void on_update(ExampleApp *app, void *, float dt)
{
   g_time += dt;
   MonstroContext *ctx = app->ui;

   monstro_set_next_window_pos(ctx, 50, 50, MONSTRO_COND_FIRST_USE_EVER);
   monstro_set_next_window_size(ctx, 550, 800, MONSTRO_COND_FIRST_USE_EVER);

   if (monstro_begin_window(ctx, "Plots", nullptr, MONSTRO_WINDOW_DEFAULT))
   {
      plot_draw(&g_line_plot, ctx);
      monstro_separator(ctx);
      plot_draw(&g_bar_plot, ctx);
      monstro_separator(ctx);
      plot_draw(&g_scatter_plot, ctx);
      monstro_end_window(ctx);
   }
}

int main(int argc, char *argv[])
{
   (void)argc;
   (void)argv;
   ExampleAppDesc desc = {};
   desc.title = "Monstro Plot Example";
   desc.width = 700;
   desc.height = 900;
   desc.init = on_init;
   desc.update = on_update;
   return example_app_run(desc);
}

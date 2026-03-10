#include "../common/example_app.h"
#include "curve.h"

static CurveEditor g_curve;

static void on_init(ExampleApp *, void *)
{
   curve_editor_init(&g_curve, 500, 350);
   curve_editor_add_point(&g_curve, 0.0f, 0.0f);
   curve_editor_add_point(&g_curve, 0.25f, 0.7f);
   curve_editor_add_point(&g_curve, 0.5f, 0.3f);
   curve_editor_add_point(&g_curve, 0.75f, 0.9f);
   curve_editor_add_point(&g_curve, 1.0f, 1.0f);
}

static void on_update(ExampleApp *app, void *, float)
{
   MonstroContext *ctx = app->ui;
   monstro_set_next_window_pos(ctx, 50, 50, MONSTRO_COND_FIRST_USE_EVER);
   monstro_set_next_window_size(ctx, 560, 450, MONSTRO_COND_FIRST_USE_EVER);

   if (monstro_begin_window(ctx, "Curve Editor", nullptr, MONSTRO_WINDOW_DEFAULT))
   {
      curve_editor_draw(&g_curve, ctx);
      monstro_end_window(ctx);
   }
}

int main(int argc, char *argv[])
{
   (void)argc;
   (void)argv;
   ExampleAppDesc desc = {};
   desc.title = "Monstro Curve Editor Example";
   desc.width = 700;
   desc.height = 550;
   desc.init = on_init;
   desc.update = on_update;
   return example_app_run(desc);
}

#include "../common/example_app.h"
#include "toast.h"
#include <cstdio>

static ToastManager g_toasts;
static float g_timer = 0.0f;

static void on_init(ExampleApp *, void *)
{
   toast_init(&g_toasts);
   toast_show(&g_toasts, TOAST_INFO, 4.0f, "Welcome to the toast example!");
}

static void on_update(ExampleApp *app, void *, float dt)
{
   MonstroContext *ctx = app->ui;
   g_timer += dt;

   monstro_set_next_window_pos(ctx, 50, 50, MONSTRO_COND_FIRST_USE_EVER);
   monstro_set_next_window_size(ctx, 300, 200, MONSTRO_COND_FIRST_USE_EVER);

   if (monstro_begin_window(ctx, "Toast Controls", nullptr, MONSTRO_WINDOW_DEFAULT))
   {
      if (monstro_button(ctx, "Info Toast", nullptr))
         toast_show(&g_toasts, TOAST_INFO, 3.0f, "Information message #%.0f", g_timer);
      if (monstro_button(ctx, "Success Toast", nullptr))
         toast_show(&g_toasts, TOAST_SUCCESS, 3.0f, "Operation completed successfully!");
      if (monstro_button(ctx, "Warning Toast", nullptr))
         toast_show(&g_toasts, TOAST_WARNING, 4.0f, "Warning: disk space is running low");
      if (monstro_button(ctx, "Error Toast", nullptr))
         toast_show(&g_toasts, TOAST_ERROR, 5.0f, "Error: connection timed out");
      monstro_end_window(ctx);
   }

   toast_draw(&g_toasts, ctx, dt);
}

int main(int argc, char *argv[])
{
   (void)argc;
   (void)argv;
   ExampleAppDesc desc = {};
   desc.title = "Monstro Toast Example";
   desc.width = 800;
   desc.height = 600;
   desc.init = on_init;
   desc.update = on_update;
   return example_app_run(desc);
}

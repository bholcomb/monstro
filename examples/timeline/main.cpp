#include "../common/example_app.h"
#include "timeline.h"

static Timeline g_timeline;

static void on_init(ExampleApp *, void *)
{
   timeline_init(&g_timeline, 30.0f);
   timeline_add_track(&g_timeline, "Audio");
   timeline_add_track(&g_timeline, "Video");
   timeline_add_track(&g_timeline, "Effects");
   timeline_add_track(&g_timeline, "Text");

   timeline_add_clip(&g_timeline, 0, "Intro Music", 0.0f, 5.0f, 0xFF4488AA);
   timeline_add_clip(&g_timeline, 0, "Background", 5.0f, 15.0f, 0xFF448866);
   timeline_add_clip(&g_timeline, 0, "Outro", 22.0f, 8.0f, 0xFF6644AA);
   timeline_add_clip(&g_timeline, 1, "Clip A", 0.0f, 8.0f, 0xFFAA4444);
   timeline_add_clip(&g_timeline, 1, "Clip B", 8.5f, 6.0f, 0xFFAA6644);
   timeline_add_clip(&g_timeline, 1, "Clip C", 15.0f, 10.0f, 0xFFAAAA44);
   timeline_add_clip(&g_timeline, 2, "Fade In", 0.0f, 2.0f, 0xFF44AA88);
   timeline_add_clip(&g_timeline, 2, "Transition", 8.0f, 1.5f, 0xFF4488AA);
   timeline_add_clip(&g_timeline, 2, "Fade Out", 28.0f, 2.0f, 0xFF44AA88);
   timeline_add_clip(&g_timeline, 3, "Title", 1.0f, 4.0f, 0xFFAA44AA);
   timeline_add_clip(&g_timeline, 3, "Subtitle", 10.0f, 5.0f, 0xFF8844AA);
}

static void on_update(ExampleApp *app, void *, float dt) { timeline_draw(&g_timeline, app->ui, dt); }

int main(int argc, char *argv[])
{
   (void)argc;
   (void)argv;
   ExampleAppDesc desc = {};
   desc.title = "Monstro Timeline Example";
   desc.width = 900;
   desc.height = 400;
   desc.init = on_init;
   desc.update = on_update;
   return example_app_run(desc);
}

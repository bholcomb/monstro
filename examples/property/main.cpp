#include "../common/example_app.h"
#include "property.h"

static PropertyEditor g_editor;

static void on_init(ExampleApp *, void *)
{
   property_editor_init(&g_editor);

   property_editor_add_section(&g_editor, "Transform");
   property_editor_add_float(&g_editor, "Position X", 0.0f, -100.0f, 100.0f);
   property_editor_add_float(&g_editor, "Position Y", 0.0f, -100.0f, 100.0f);
   property_editor_add_float(&g_editor, "Position Z", 0.0f, -100.0f, 100.0f);
   property_editor_add_float(&g_editor, "Rotation", 0.0f, 0.0f, 360.0f);
   property_editor_add_float(&g_editor, "Scale", 1.0f, 0.01f, 10.0f);

   property_editor_add_section(&g_editor, "Rendering");
   property_editor_add_bool(&g_editor, "Visible", true);
   property_editor_add_bool(&g_editor, "Cast Shadows", true);
   property_editor_add_color(&g_editor, "Tint", MONSTRO_COLOR_WHITE);
   property_editor_add_float(&g_editor, "Opacity", 1.0f, 0.0f, 1.0f);

   const char *blend_modes[] = {"Opaque", "Alpha Blend", "Additive", "Multiply"};
   property_editor_add_enum(&g_editor, "Blend Mode", 0, blend_modes, 4);

   property_editor_add_section(&g_editor, "Metadata");
   property_editor_add_string(&g_editor, "Name", "GameObject_01");
   property_editor_add_string(&g_editor, "Tag", "default");
   property_editor_add_int(&g_editor, "Layer", 0, 0, 31);
}

static void on_update(ExampleApp *app, void *, float) { property_editor_draw(&g_editor, app->ui); }

int main(int argc, char *argv[])
{
   (void)argc;
   (void)argv;
   ExampleAppDesc desc = {};
   desc.title = "Monstro Property Editor Example";
   desc.width = 600;
   desc.height = 700;
   desc.init = on_init;
   desc.update = on_update;
   return example_app_run(desc);
}

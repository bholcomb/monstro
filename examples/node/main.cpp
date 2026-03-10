#include "../common/example_app.h"
#include "node.h"

static NodeEditor g_nodes;

static void on_init(ExampleApp *, void *)
{
   node_editor_init(&g_nodes);

   int n0 = node_editor_add_node(&g_nodes, "Texture", 50, 100, 0xFF446688);
   node_editor_add_port(&g_nodes, n0, "UV", false);
   node_editor_add_port(&g_nodes, n0, "Color", true);
   node_editor_add_port(&g_nodes, n0, "Alpha", true);

   int n1 = node_editor_add_node(&g_nodes, "Math", 300, 50, 0xFF668844);
   node_editor_add_port(&g_nodes, n1, "A", false);
   node_editor_add_port(&g_nodes, n1, "B", false);
   node_editor_add_port(&g_nodes, n1, "Result", true);

   int n2 = node_editor_add_node(&g_nodes, "Output", 550, 150, 0xFF884466);
   node_editor_add_port(&g_nodes, n2, "Color", false);
   node_editor_add_port(&g_nodes, n2, "Normal", false);
   node_editor_add_port(&g_nodes, n2, "Roughness", false);

   int n3 = node_editor_add_node(&g_nodes, "Noise", 50, 300, 0xFF664488);
   node_editor_add_port(&g_nodes, n3, "Scale", false);
   node_editor_add_port(&g_nodes, n3, "Value", true);

   node_editor_add_link(&g_nodes, n0, 1, n1, 0);
   node_editor_add_link(&g_nodes, n1, 2, n2, 0);
   node_editor_add_link(&g_nodes, n3, 1, n1, 1);
}

static void on_update(ExampleApp *app, void *, float) { node_editor_draw(&g_nodes, app->ui); }

int main(int argc, char *argv[])
{
   (void)argc;
   (void)argv;
   ExampleAppDesc desc = {};
   desc.title = "Monstro Node Editor Example";
   desc.width = 900;
   desc.height = 700;
   desc.init = on_init;
   desc.update = on_update;
   return example_app_run(desc);
}

#ifndef EXAMPLE_NODE_H
#define EXAMPLE_NODE_H

#include <monstro.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define NODE_MAX_NODES 64
#define NODE_MAX_PORTS 8
#define NODE_MAX_LINKS 128
#define NODE_MAX_NAME 64

typedef struct NodePort
{
   char name[NODE_MAX_NAME];
   bool is_output;
} NodePort;

typedef struct Node
{
   char name[NODE_MAX_NAME];
   float x, y;
   float w, h;
   uint32_t color;

   NodePort ports[NODE_MAX_PORTS];
   int port_count;
} Node;

typedef struct NodeLink
{
   int src_node, src_port;
   int dst_node, dst_port;
} NodeLink;

typedef struct NodeEditor
{
   Node nodes[NODE_MAX_NODES];
   int node_count;

   NodeLink links[NODE_MAX_LINKS];
   int link_count;

   float pan_x, pan_y;
   int selected_node;
   int dragging_node;
   float drag_offset_x, drag_offset_y;

   int linking_from_node, linking_from_port;
   bool is_linking;
} NodeEditor;

void node_editor_init(NodeEditor *ne);
int node_editor_add_node(NodeEditor *ne, const char *name, float x, float y, uint32_t color);
void node_editor_add_port(NodeEditor *ne, int node_idx, const char *name, bool is_output);
int node_editor_add_link(NodeEditor *ne, int src_node, int src_port, int dst_node, int dst_port);
void node_editor_draw(NodeEditor *ne, MonstroContext *ctx);

#ifdef __cplusplus
}
#endif

#endif

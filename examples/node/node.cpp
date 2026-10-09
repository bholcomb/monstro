#include "node.h"
#include <cmath>
#include <cstdio>
#include <cstring>

void node_editor_init(NodeEditor *ne)
{
   memset(ne, 0, sizeof(*ne));
   ne->selected_node = -1;
   ne->dragging_node = -1;
   ne->linking_from_node = -1;
}

int node_editor_add_node(NodeEditor *ne, const char *name, float x, float y, uint32_t color)
{
   if (ne->node_count >= NODE_MAX_NODES)
      return -1;
   int i = ne->node_count++;
   strncpy(ne->nodes[i].name, name, NODE_MAX_NAME - 1);
   ne->nodes[i].x = x;
   ne->nodes[i].y = y;
   ne->nodes[i].w = 150.0f;
   ne->nodes[i].h = 0;
   ne->nodes[i].color = color;
   return i;
}

void node_editor_add_port(NodeEditor *ne, int ni, const char *name, bool is_output)
{
   if (ni < 0 || ni >= ne->node_count)
      return;
   Node *n = &ne->nodes[ni];
   if (n->port_count >= NODE_MAX_PORTS)
      return;
   NodePort *p = &n->ports[n->port_count++];
   strncpy(p->name, name, NODE_MAX_NAME - 1);
   p->is_output = is_output;
}

int node_editor_add_link(NodeEditor *ne, int sn, int sp, int dn, int dp)
{
   if (ne->link_count >= NODE_MAX_LINKS)
      return -1;
   int i = ne->link_count++;
   ne->links[i].src_node = sn;
   ne->links[i].src_port = sp;
   ne->links[i].dst_node = dn;
   ne->links[i].dst_port = dp;
   return i;
}

static float dist2(float ax, float ay, float bx, float by)
{
   float dx = ax - bx, dy = ay - by;
   return dx * dx + dy * dy;
}

struct PortPos
{
   float x, y;
};

static PortPos get_port_pos(Node *n, int port_idx, float pan_x, float pan_y)
{
   float header_h = 24.0f;
   float port_h = 20.0f;
   float py = n->y + pan_y + header_h + (float)port_idx * port_h + port_h * 0.5f;
   PortPos pp;
   if (n->ports[port_idx].is_output)
   {
      pp.x = n->x + pan_x + n->w;
   }
   else
   {
      pp.x = n->x + pan_x;
   }
   pp.y = py;
   return pp;
}

void node_editor_draw(NodeEditor *ne, MonstroContext *ctx)
{
   monstro_set_next_window_pos(ctx, 50, 50, MONSTRO_COND_FIRST_USE_EVER);
   monstro_set_next_window_size(ctx, 800, 600, MONSTRO_COND_FIRST_USE_EVER);

   if (monstro_begin_window(ctx, "Node Editor", nullptr, MONSTRO_WINDOW_DEFAULT))
   {
      if (monstro_is_window_collapsed(ctx))
      {
         monstro_end_window(ctx);
         return;
      }

      monstro_add_item(ctx, 0, 2);
      MonstroVec2 origin = monstro_get_cursor_screen_pos(ctx);
      float area_w = 750.0f, area_h = 500.0f;
      MonstroRect area = {origin.x, origin.y, area_w, area_h};

      MonstroCanvas *canvas = monstro_get_window_canvas(ctx);
      MonstroFont *font = monstro_get_style(ctx)->font;
      monstro_draw_rect_filled(canvas, area, 0xFF1A1A2A, 0, MONSTRO_CORNER_NONE);

      float grid_step = 20.0f;
      float gx_off = fmodf(ne->pan_x, grid_step);
      float gy_off = fmodf(ne->pan_y, grid_step);
      for (float gx = origin.x + gx_off; gx < origin.x + area_w; gx += grid_step)
      {
         monstro_draw_line(canvas, {gx, origin.y}, {gx, origin.y + area_h}, 0x15FFFFFF, 1.0f);
      }
      for (float gy = origin.y + gy_off; gy < origin.y + area_h; gy += grid_step)
      {
         monstro_draw_line(canvas, {origin.x, gy}, {origin.x + area_w, gy}, 0x15FFFFFF, 1.0f);
      }

      monstro_push_clip_rect(canvas, area);

      for (int l = 0; l < ne->link_count; l++)
      {
         NodeLink *lnk = &ne->links[l];
         if (lnk->src_node < 0 || lnk->src_node >= ne->node_count)
            continue;
         if (lnk->dst_node < 0 || lnk->dst_node >= ne->node_count)
            continue;

         PortPos sp =
             get_port_pos(&ne->nodes[lnk->src_node], lnk->src_port, origin.x + ne->pan_x, origin.y + ne->pan_y);
         PortPos dp =
             get_port_pos(&ne->nodes[lnk->dst_node], lnk->dst_port, origin.x + ne->pan_x, origin.y + ne->pan_y);

         float dx = fabsf(dp.x - sp.x) * 0.5f;
         MonstroVec2 p1 = {sp.x, sp.y};
         MonstroVec2 p2 = {sp.x + dx, sp.y};
         MonstroVec2 p3 = {dp.x - dx, dp.y};
         MonstroVec2 p4 = {dp.x, dp.y};
         monstro_draw_bezier_cubic(canvas, p1, p2, p3, p4, 0xFFAABBCC, 2.0f, 24);
      }

      float header_h = 24.0f;
      float port_h = 20.0f;
      float port_radius = 5.0f;

      for (int i = 0; i < ne->node_count; i++)
      {
         Node *n = &ne->nodes[i];
         float nx = origin.x + ne->pan_x + n->x;
         float ny = origin.y + ne->pan_y + n->y;
         n->h = header_h + (float)n->port_count * port_h + 4.0f;

         MonstroRect node_rect = {nx, ny, n->w, n->h};
         uint32_t bg = (i == ne->selected_node) ? 0xFF333355 : 0xFF2A2A3E;
         monstro_draw_rect_filled(canvas, node_rect, bg, 6.0f, MONSTRO_CORNER_ALL);

         MonstroRect header_rect = {nx, ny, n->w, header_h};
         monstro_draw_rect_filled(canvas, header_rect, n->color, 6.0f,
                                  (MonstroCornerFlags)(MONSTRO_CORNER_UL | MONSTRO_CORNER_UR));
         monstro_draw_text(canvas, font, {nx + 6, ny + 5}, 0xFFFFFFFF, n->name);

         uint32_t border = (i == ne->selected_node) ? 0xFFAAAAFF : 0x60FFFFFF;
         monstro_draw_rect(canvas, node_rect, border, 6.0f, MONSTRO_CORNER_ALL, 1.0f);

         for (int p = 0; p < n->port_count; p++)
         {
            PortPos pp = get_port_pos(n, p, origin.x + ne->pan_x, origin.y + ne->pan_y);
            MonstroVec2 pc = {pp.x, pp.y};
            uint32_t port_col = n->ports[p].is_output ? 0xFF44AAFF : 0xFF44FF88;
            monstro_draw_circle_filled(canvas, pc, port_radius, port_col, 12);
            monstro_draw_circle(canvas, pc, port_radius, 0xFFFFFFFF, 12, 1.0f);

            float text_x = n->ports[p].is_output ? (pp.x - 60.0f) : (pp.x + 8.0f);
            monstro_draw_text(canvas, font, {text_x, pp.y - 6}, 0xFFCCCCCC, n->ports[p].name);
         }
      }

      if (ne->is_linking && ne->linking_from_node >= 0)
      {
         PortPos sp = get_port_pos(&ne->nodes[ne->linking_from_node], ne->linking_from_port, origin.x + ne->pan_x,
                                   origin.y + ne->pan_y);
         MonstroVec2 mpos = monstro_get_mouse_pos(ctx);
         float mx = mpos.x, my = mpos.y;
         float dx = fabsf(mx - sp.x) * 0.5f;
         MonstroVec2 p1 = {sp.x, sp.y};
         MonstroVec2 p2 = {sp.x + dx, sp.y};
         MonstroVec2 p3 = {mx - dx, my};
         MonstroVec2 p4 = {mx, my};
         monstro_draw_bezier_cubic(canvas, p1, p2, p3, p4, 0x80FFFFFF, 2.0f, 24);
      }

      monstro_pop_clip_rect(canvas);
      monstro_draw_rect(canvas, area, 0x60FFFFFF, 0, MONSTRO_CORNER_NONE, 1.0f);

      if (monstro_invisible_button(ctx, "##node_area", area_w, area_h))
      {
      }

      MonstroVec2 mp = monstro_get_mouse_pos(ctx);
      float mx = mp.x, my = mp.y;

      if (monstro_is_item_hovered(ctx) && monstro_is_mouse_clicked(ctx, MONSTRO_MOUSE_BUTTON_LEFT, false))
      {
         ne->selected_node = -1;
         ne->dragging_node = -1;

         for (int i = 0; i < ne->node_count; i++)
         {
            Node *n = &ne->nodes[i];
            for (int p = 0; p < n->port_count; p++)
            {
               PortPos pp = get_port_pos(n, p, origin.x + ne->pan_x, origin.y + ne->pan_y);
               if (dist2(mx, my, pp.x, pp.y) < port_radius * port_radius * 4)
               {
                  ne->is_linking = true;
                  ne->linking_from_node = i;
                  ne->linking_from_port = p;
                  goto done_click;
               }
            }
         }

         for (int i = ne->node_count - 1; i >= 0; i--)
         {
            Node *n = &ne->nodes[i];
            float nx = origin.x + ne->pan_x + n->x;
            float ny = origin.y + ne->pan_y + n->y;
            if (mx >= nx && mx <= nx + n->w && my >= ny && my <= ny + n->h)
            {
               ne->selected_node = i;
               ne->dragging_node = i;
               ne->drag_offset_x = mx - nx;
               ne->drag_offset_y = my - ny;
               break;
            }
         }
      done_click:;
      }

      if (ne->is_linking && monstro_is_mouse_released(ctx, MONSTRO_MOUSE_BUTTON_LEFT))
      {
         for (int i = 0; i < ne->node_count; i++)
         {
            Node *n = &ne->nodes[i];
            for (int p = 0; p < n->port_count; p++)
            {
               if (i == ne->linking_from_node)
                  continue;
               PortPos pp = get_port_pos(n, p, origin.x + ne->pan_x, origin.y + ne->pan_y);
               if (dist2(mx, my, pp.x, pp.y) < port_radius * port_radius * 4)
               {
                  node_editor_add_link(ne, ne->linking_from_node, ne->linking_from_port, i, p);
                  break;
               }
            }
         }
         ne->is_linking = false;
         ne->linking_from_node = -1;
      }

      if (ne->dragging_node >= 0 && monstro_is_mouse_down(ctx, MONSTRO_MOUSE_BUTTON_LEFT))
      {
         ne->nodes[ne->dragging_node].x = mx - ne->drag_offset_x - origin.x - ne->pan_x;
         ne->nodes[ne->dragging_node].y = my - ne->drag_offset_y - origin.y - ne->pan_y;
      }

      if (monstro_is_mouse_released(ctx, MONSTRO_MOUSE_BUTTON_LEFT))
      {
         ne->dragging_node = -1;
      }

      // Not in the frame the button is pressed: a move taken with the press only brings the mouse there.
      if (monstro_is_item_hovered(ctx) && monstro_is_mouse_down(ctx, MONSTRO_MOUSE_BUTTON_MIDDLE) &&
          !monstro_is_mouse_clicked(ctx, MONSTRO_MOUSE_BUTTON_MIDDLE, false))
      {
         MonstroVec2 md = monstro_get_mouse_delta(ctx);
         ne->pan_x += md.x;
         ne->pan_y += md.y;
      }
      monstro_end_window(ctx);
   }
}

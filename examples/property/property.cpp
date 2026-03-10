#include "property.h"
#include <cstdio>
#include <cstring>

void property_editor_init(PropertyEditor *pe)
{
   memset(pe, 0, sizeof(*pe));
   pe->label_width = 120.0f;
}

static int add_item(PropertyEditor *pe)
{
   if (pe->item_count >= PROP_MAX_ITEMS)
      return -1;
   return pe->item_count++;
}

int property_editor_add_section(PropertyEditor *pe, const char *name)
{
   int i = add_item(pe);
   if (i < 0)
      return i;
   strncpy(pe->items[i].name, name, PROP_MAX_NAME - 1);
   pe->items[i].type = PROP_SECTION;
   pe->items[i].section_open = true;
   return i;
}

int property_editor_add_bool(PropertyEditor *pe, const char *name, bool value)
{
   int i = add_item(pe);
   if (i < 0)
      return i;
   strncpy(pe->items[i].name, name, PROP_MAX_NAME - 1);
   pe->items[i].type = PROP_BOOL;
   pe->items[i].bool_val = value;
   return i;
}

int property_editor_add_int(PropertyEditor *pe, const char *name, int value, int min_val, int max_val)
{
   int i = add_item(pe);
   if (i < 0)
      return i;
   strncpy(pe->items[i].name, name, PROP_MAX_NAME - 1);
   pe->items[i].type = PROP_INT;
   pe->items[i].int_val = value;
   pe->items[i].int_min = min_val;
   pe->items[i].int_max = max_val;
   return i;
}

int property_editor_add_float(PropertyEditor *pe, const char *name, float value, float min_val, float max_val)
{
   int i = add_item(pe);
   if (i < 0)
      return i;
   strncpy(pe->items[i].name, name, PROP_MAX_NAME - 1);
   pe->items[i].type = PROP_FLOAT;
   pe->items[i].float_val = value;
   pe->items[i].float_min = min_val;
   pe->items[i].float_max = max_val;
   return i;
}

int property_editor_add_string(PropertyEditor *pe, const char *name, const char *value)
{
   int i = add_item(pe);
   if (i < 0)
      return i;
   strncpy(pe->items[i].name, name, PROP_MAX_NAME - 1);
   pe->items[i].type = PROP_STRING;
   strncpy(pe->items[i].str_val, value ? value : "", PROP_MAX_STR - 1);
   return i;
}

int property_editor_add_color(PropertyEditor *pe, const char *name, MonstroColor value)
{
   int i = add_item(pe);
   if (i < 0)
      return i;
   strncpy(pe->items[i].name, name, PROP_MAX_NAME - 1);
   pe->items[i].type = PROP_COLOR;
   pe->items[i].color_val = value;
   return i;
}

int property_editor_add_enum(PropertyEditor *pe, const char *name, int current, const char *const *options, int count)
{
   int i = add_item(pe);
   if (i < 0)
      return i;
   strncpy(pe->items[i].name, name, PROP_MAX_NAME - 1);
   pe->items[i].type = PROP_ENUM;
   pe->items[i].enum_val.current = current;
   pe->items[i].enum_val.option_count = count < PROP_MAX_ENUM_OPTIONS ? count : PROP_MAX_ENUM_OPTIONS;
   for (int j = 0; j < pe->items[i].enum_val.option_count; j++)
   {
      strncpy(pe->items[i].enum_val.options[j].label, options[j], 63);
   }
   return i;
}

bool property_editor_draw(PropertyEditor *pe, MonstroContext *ctx)
{
   bool any_changed = false;
   bool in_section = true;

   monstro_set_next_window_pos(ctx, 50, 50, MONSTRO_COND_FIRST_USE_EVER);
   monstro_set_next_window_size(ctx, 400, 500, MONSTRO_COND_FIRST_USE_EVER);

   if (monstro_begin_window(ctx, "Properties", nullptr, MONSTRO_WINDOW_DEFAULT))
   {

      for (int i = 0; i < pe->item_count; i++)
      {
         PropertyItem *item = &pe->items[i];

         if (item->type == PROP_SECTION)
         {
            if (monstro_collapsing_header(ctx, item->name, &item->section_open))
            {
               in_section = true;
            }
            else
            {
               in_section = false;
            }
            continue;
         }

         if (!in_section)
            continue;

         monstro_push_id(ctx, item->name);

         monstro_begin_columns(ctx, "##prop_cols", 2);

         monstro_label(ctx, item->name);
         monstro_next_column(ctx);

         switch (item->type)
         {
         case PROP_BOOL:
            if (monstro_checkbox(ctx, "##v", &item->bool_val, nullptr))
               any_changed = true;
            break;

         case PROP_INT:
            if (monstro_slider_int(ctx, "##v", &item->int_val, item->int_min, item->int_max, "%d", nullptr))
               any_changed = true;
            break;

         case PROP_FLOAT:
            if (monstro_slider_float(ctx, "##v", &item->float_val, item->float_min, item->float_max, "%.3f", nullptr))
               any_changed = true;
            break;

         case PROP_STRING:
            if (monstro_input_text(ctx, "##v", item->str_val, PROP_MAX_STR, MONSTRO_INPUT_TEXT_NONE, nullptr))
               any_changed = true;
            break;

         case PROP_COLOR:
            if (monstro_color_button(ctx, "##v", item->color_val, 40, 20))
               any_changed = true;
            break;

         case PROP_ENUM:
         {
            const char *options[PROP_MAX_ENUM_OPTIONS];
            for (int j = 0; j < item->enum_val.option_count; j++)
               options[j] = item->enum_val.options[j].label;
            if (monstro_combo(ctx, "##v", &item->enum_val.current, options, item->enum_val.option_count, nullptr))
               any_changed = true;
            break;
         }

         default:
            break;
         }

         monstro_end_columns(ctx);
         monstro_pop_id(ctx);
      }
      monstro_end_window(ctx);
   }
   return any_changed;
}

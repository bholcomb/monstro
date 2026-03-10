#ifndef EXAMPLE_PROPERTY_H
#define EXAMPLE_PROPERTY_H

#include <monstro.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum PropertyType
{
   PROP_BOOL,
   PROP_INT,
   PROP_FLOAT,
   PROP_STRING,
   PROP_COLOR,
   PROP_ENUM,
   PROP_SECTION
} PropertyType;

#define PROP_MAX_NAME 64
#define PROP_MAX_STR 256
#define PROP_MAX_ITEMS 128
#define PROP_MAX_ENUM_OPTIONS 16

typedef struct PropertyEnumOption
{
   char label[64];
} PropertyEnumOption;

typedef struct PropertyItem
{
   char name[PROP_MAX_NAME];
   PropertyType type;
   union
   {
      bool bool_val;
      int int_val;
      float float_val;
      char str_val[PROP_MAX_STR];
      MonstroColor color_val;
      struct
      {
         int current;
         PropertyEnumOption options[PROP_MAX_ENUM_OPTIONS];
         int option_count;
      } enum_val;
   };
   float float_min, float_max;
   int int_min, int_max;
   bool section_open;
} PropertyItem;

typedef struct PropertyEditor
{
   PropertyItem items[PROP_MAX_ITEMS];
   int item_count;
   float label_width;
} PropertyEditor;

void property_editor_init(PropertyEditor *pe);
int property_editor_add_section(PropertyEditor *pe, const char *name);
int property_editor_add_bool(PropertyEditor *pe, const char *name, bool value);
int property_editor_add_int(PropertyEditor *pe, const char *name, int value, int min_val, int max_val);
int property_editor_add_float(PropertyEditor *pe, const char *name, float value, float min_val, float max_val);
int property_editor_add_string(PropertyEditor *pe, const char *name, const char *value);
int property_editor_add_color(PropertyEditor *pe, const char *name, MonstroColor value);
int property_editor_add_enum(PropertyEditor *pe, const char *name, int current, const char *const *options, int count);
bool property_editor_draw(PropertyEditor *pe, MonstroContext *ctx);

#ifdef __cplusplus
}
#endif

#endif

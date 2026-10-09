#include "example_app.h"
#include <monstro/icons.h>
#include <monstro/widgets.h>

struct BasicDemo
{
   int buttonClicks;
   bool checkboxValue;
   float sliderValue;
   float sliderValue2;
   int sliderInt;
   float knobValue;
   float knobValue2;
   int radioValue;
   int comboSelected;

   char textInput[256];
   char multilineInput[1024];
   int intInput;
   float floatInput;
   double doubleInput;

   char tableNames[3][64];
   float tableValues[3];

   bool showModal;
   bool secondWindowOpen;

   MonstroColor pickerColor;
   bool colorPickerOpen;

   MonstroFileDialogState *fileDialog;
   char lastFilePath[4096];

   VETexture iconAtlasTexture;
   uint64_t iconAtlasTextureId;
   MonstroIconId firstIconId;
};

static void basic_init(ExampleApp *app, void *ud)
{
   BasicDemo *d = (BasicDemo *)ud;

   constexpr int icon_w = 16, icon_h = 16;
   constexpr int cols = 4, rows = 1;
   constexpr int atlas_w = icon_w * cols, atlas_h = icon_h * rows;
   uint32_t pixels[atlas_w * atlas_h];

   uint32_t colors[4] = {0xFF4444FF, 0xFF44FF44, 0xFFFF8844, 0xFFFFFF44};
   for (int c = 0; c < cols; c++)
   {
      for (int y = 0; y < icon_h; y++)
      {
         for (int x = 0; x < icon_w; x++)
         {
            pixels[y * atlas_w + c * icon_w + x] = colors[c];
         }
      }
   }

   VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_HOST_TRANSFER_BIT_EXT;
   ea_check(veCreateTexture2D(app->device, atlas_w, atlas_h, VK_FORMAT_R8G8B8A8_UNORM, usage, "BasicIconAtlas",
                              &d->iconAtlasTexture),
            "veCreateTexture2D(icons)");
   ea_check(veHostWriteTextureRegion(app->device, d->iconAtlasTexture, pixels, sizeof(pixels), nullptr),
            "veHostWriteTextureRegion(icons)");
   d->iconAtlasTextureId = (uint64_t)veGetDefaultTextureView(app->device, d->iconAtlasTexture);
   d->firstIconId = 1;
   monstro_register_icon_atlas(app->env, d->firstIconId, d->iconAtlasTextureId, icon_w, icon_h, atlas_w, atlas_h, cols,
                               rows);
}

static void basic_update(ExampleApp *app, void *ud, float dt)
{
   (void)dt;
   BasicDemo *d = (BasicDemo *)ud;
   MonstroContext *ctx = app->ui;

   monstro_set_next_window_pos(ctx, 50.0f, 50.0f, MONSTRO_COND_FIRST_USE_EVER);
   monstro_set_next_window_size(ctx, 450.0f, 900.0f, MONSTRO_COND_FIRST_USE_EVER);

   MonstroWindowFlags windowFlags = (MonstroWindowFlags)(MONSTRO_WINDOW_DEFAULT | MONSTRO_WINDOW_MENU_BAR);
   if (monstro_begin_window(ctx, "Monstro Demo", nullptr, windowFlags))
   {

      if (monstro_begin_menu_bar(ctx))
      {
         if (monstro_begin_menu(ctx, "File", true))
         {
            if (monstro_menu_item(ctx, "Load", nullptr, true))
            {
               printf("Menu: Load clicked\n");
            }
            if (monstro_menu_item(ctx, "Save", nullptr, true))
            {
               printf("Menu: Save clicked\n");
            }
            if (monstro_begin_menu(ctx, "Options", true))
            {
               if (monstro_menu_item(ctx, "Option 1", nullptr, true))
               {
                  printf("Menu: Option 1 clicked\n");
               }
               if (monstro_menu_item(ctx, "Option 2", nullptr, true))
               {
                  printf("Menu: Option 2 clicked\n");
               }
               monstro_end_menu(ctx);
            }
            monstro_separator(ctx);
            if (monstro_menu_item(ctx, "Exit", nullptr, true))
            {
               printf("Menu: Exit clicked\n");
            }
            monstro_end_menu(ctx);
         }
         if (monstro_begin_menu(ctx, "Edit", true))
         {
            if (monstro_menu_item(ctx, "Cut", nullptr, true))
            {
               printf("Menu: Cut clicked\n");
            }
            if (monstro_menu_item(ctx, "Copy", nullptr, true))
            {
               printf("Menu: Copy clicked\n");
            }
            if (monstro_menu_item(ctx, "Paste", nullptr, true))
            {
               printf("Menu: Paste clicked\n");
            }
            monstro_end_menu(ctx);
         }
         monstro_end_menu_bar(ctx);
      }

      monstro_label(ctx, "Hello, Monstro!");
      monstro_separator(ctx);

      if (monstro_button(ctx, "Click Me!", "Click this button"))
      {
         d->buttonClicks++;
         printf("Button clicked! Total: %d\n", d->buttonClicks);
      }

      if (monstro_checkbox(ctx, "Enable Feature", &d->checkboxValue, "Toggle this feature"))
      {
         printf("Checkbox toggled: %d\n", d->checkboxValue);
      }

      monstro_separator_text(ctx, "Icons");

      if (d->firstIconId)
      {
         char ib[32];

         monstro_begin_layout(ctx, MONSTRO_LAYOUT_HORIZONTAL);
         {
            monstro_icon(ctx, d->firstIconId, 16.0f);
            monstro_icon(ctx, d->firstIconId + 1, 16.0f);
            monstro_icon(ctx, d->firstIconId + 2, 16.0f);
            monstro_icon(ctx, d->firstIconId + 3, 16.0f);
         }
         monstro_end_layout(ctx);

         char btn_label[64];
         monstro_begin_layout(ctx, MONSTRO_LAYOUT_HORIZONTAL);
         {
            snprintf(btn_label, sizeof(btn_label), "btn1##%sSave", monstro_icon_str(d->firstIconId, ib, sizeof(ib)));
            if (monstro_button(ctx, btn_label, nullptr))
            {
               printf("Icon button Save clicked\n");
            }

            snprintf(btn_label, sizeof(btn_label), "btn2##%sOpen",
                     monstro_icon_str(d->firstIconId + 1, ib, sizeof(ib)));
            if (monstro_button(ctx, btn_label, nullptr))
            {
               printf("Icon button Open clicked\n");
            }

            snprintf(btn_label, sizeof(btn_label), "btn3##%sExport",
                     monstro_icon_str(d->firstIconId + 2, ib, sizeof(ib)));
            if (monstro_button(ctx, btn_label, nullptr))
            {
               printf("Icon button Export clicked\n");
            }
         }
         monstro_end_layout(ctx);
      }

      monstro_separator_text(ctx, "Sliders");

      monstro_slider_float(ctx, "Volume", &d->sliderValue, 0.0f, 1.0f, "%.2f", "Adjust volume");
      monstro_slider_float(ctx, "Speed", &d->sliderValue2, 0.0f, 100.0f, "%.1f", "Adjust speed");
      monstro_slider_int(ctx, "Count", &d->sliderInt, 0, 10, "%d", "Set count");

      monstro_separator_text(ctx, "Knobs");
      monstro_knob(ctx, "Gain", &d->knobValue, 0.0f, 1.0f, "%.2f", "Adjust gain");
      monstro_knob(ctx, "Rate", &d->knobValue2, 0.0f, 100.0f, "%.1f", "Adjust rate");

      monstro_separator_text(ctx, "Progress Bars");
      char gain_text[32];
      snprintf(gain_text, sizeof(gain_text), "Gain: %.2f", d->knobValue);
      monstro_progress_bar(ctx, d->knobValue, 100.0f, 20.0f, gain_text);

      monstro_progress_bar(ctx, d->knobValue2 / 100.0f, 100.0f, 20.0f, NULL);

      monstro_separator_text(ctx, "Radio Buttons");

      if (monstro_radio_button(ctx, "Option A", &d->radioValue, 0, nullptr))
      {
         printf("Radio button Option A clicked with value: %d\n", d->radioValue);
      }
      if (monstro_radio_button(ctx, "Option B", &d->radioValue, 1, nullptr))
      {
         printf("Radio button Option B clicked with value: %d\n", d->radioValue);
      }
      if (monstro_radio_button(ctx, "Option C", &d->radioValue, 2, nullptr))
      {
         printf("Radio button Option C clicked with value: %d\n", d->radioValue);
      }

      monstro_separator_text(ctx, "Combo Box");

      {
         const char *items[] = {"Apple", "Banana", "Cherry", "Date", "Elderberry"};
         if (monstro_combo(ctx, "Fruit", &d->comboSelected, items, 5, nullptr))
         {
            printf("Combo selected: %s (%d)\n", items[d->comboSelected], d->comboSelected);
         }
      }

      monstro_separator_text(ctx, "Text Input");
      monstro_input_text(ctx, "Name", d->textInput, sizeof(d->textInput), MONSTRO_INPUT_TEXT_NONE, "Enter your name");
      monstro_input_text(ctx, "Password", d->textInput, sizeof(d->textInput), MONSTRO_INPUT_TEXT_PASSWORD,
                         "Enter password");
      monstro_input_text_multiline(ctx, "Notes", d->multilineInput, sizeof(d->multilineInput), 0.0f, 80.0f,
                                   MONSTRO_INPUT_TEXT_NONE, "Enter notes");

      monstro_separator_text(ctx, "Numeric Input");
      monstro_input_int(ctx, "Count", &d->intInput, 1, 10, "Integer with +/- buttons");
      monstro_input_float(ctx, "Amount", &d->floatInput, 0.1f, 1.0f, "%.2f", "Float with +/- buttons");
      monstro_input_double(ctx, "Precise", &d->doubleInput, 0.001, 0.1, "%.6f", "Double precision");

      monstro_separator_text(ctx, "Layout Test");

      monstro_begin_layout(ctx, MONSTRO_LAYOUT_HORIZONTAL);
      {
         if (monstro_button(ctx, "Left", nullptr))
         {
            printf("Left button clicked\n");
         }
         if (monstro_button(ctx, "Middle", nullptr))
         {
            printf("Middle button clicked\n");
         }
         if (monstro_button(ctx, "Right", nullptr))
         {
            printf("Right button clicked\n");
         }
      }
      monstro_end_layout(ctx);

      monstro_separator_text(ctx, "Percent Width");

      float halfWidth = monstro_percent(ctx, 50.0f, MONSTRO_LAYOUT_HORIZONTAL);
      monstro_begin_layout(ctx, MONSTRO_LAYOUT_HORIZONTAL);
      {
         if (monstro_button_sized(ctx, "Left 50%", halfWidth - 4.0f, 25.0f, nullptr))
         {
            printf("Left 50%% button clicked\n");
         }
         if (monstro_button_sized(ctx, "Right 50%", halfWidth - 4.0f, 25.0f, nullptr))
         {
            printf("Right 50%% button clicked\n");
         }
      }
      monstro_end_layout(ctx);

      monstro_separator_text(ctx, "Tree View");

      if (monstro_begin_tree_node(ctx, "Scene", MONSTRO_TREE_DEFAULT_OPEN))
      {
         if (monstro_begin_tree_node(ctx, "Meshes", MONSTRO_TREE_NONE))
         {
            monstro_tree_leaf(ctx, "Cube", MONSTRO_TREE_NONE);
            monstro_tree_leaf(ctx, "Sphere", MONSTRO_TREE_NONE);
            monstro_tree_leaf(ctx, "Plane", MONSTRO_TREE_NONE);
            monstro_end_tree_node(ctx);
         }
         if (monstro_begin_tree_node(ctx, "Lights", MONSTRO_TREE_NONE))
         {
            monstro_tree_leaf(ctx, "Sun", MONSTRO_TREE_SELECTED);
            monstro_tree_leaf(ctx, "Point Light", MONSTRO_TREE_NONE);
            monstro_end_tree_node(ctx);
         }
         monstro_tree_leaf(ctx, "Camera", MONSTRO_TREE_NONE);
         monstro_end_tree_node(ctx);
      }

      if (monstro_collapsing_header(ctx, "Extra Settings", nullptr))
      {
         monstro_label(ctx, "These are extra settings.");
         monstro_checkbox(ctx, "Wireframe", &d->checkboxValue, nullptr);
      }

      monstro_separator_text(ctx, "Columns");

      monstro_begin_columns(ctx, "cols_equal", 3);
      monstro_label(ctx, "Column 1");
      monstro_next_column(ctx);
      monstro_label(ctx, "Column 2");
      monstro_next_column(ctx);
      monstro_label(ctx, "Column 3");
      monstro_next_column(ctx);
      monstro_end_columns(ctx);

      float col_widths[] = {100.0f, 0.0f, 80.0f};
      monstro_begin_columns_ex(ctx, "cols_form", 3, col_widths);
      monstro_label(ctx, "Name:");
      monstro_next_column(ctx);
      monstro_input_text(ctx, "col_name##Name", d->textInput, sizeof(d->textInput), MONSTRO_INPUT_TEXT_NONE, nullptr);
      monstro_next_column(ctx);
      if (monstro_button(ctx, "Clear", nullptr))
      {
         d->textInput[0] = '\0';
      }
      monstro_next_column(ctx);

      monstro_label(ctx, "Volume:");
      monstro_next_column(ctx);
      monstro_slider_float(ctx, "col_vol##Vol", &d->sliderValue, 0.0f, 1.0f, "%.2f", nullptr);
      monstro_next_column(ctx);
      if (monstro_button(ctx, "Reset##vol", nullptr))
      {
         d->sliderValue = 0.5f;
      }
      monstro_next_column(ctx);
      monstro_end_columns(ctx);

      monstro_separator_text(ctx, "Groups");

      monstro_begin_group(ctx, MONSTRO_LAYOUT_VERTICAL, MONSTRO_GROUP_BORDER);
      {
         monstro_label(ctx, "Bordered Group");
         monstro_checkbox(ctx, "Option 1", &d->checkboxValue, nullptr);
      }
      monstro_end_group(ctx);

      monstro_begin_group(ctx, MONSTRO_LAYOUT_HORIZONTAL,
                          (MonstroGroupFlags)(MONSTRO_GROUP_BORDER | MONSTRO_GROUP_BACKGROUND));
      {
         if (monstro_button(ctx, "A", nullptr))
         {
            printf("A\n");
         }
         if (monstro_button(ctx, "B", nullptr))
         {
            printf("B\n");
         }
         if (monstro_button(ctx, "C", nullptr))
         {
            printf("C\n");
         }
      }
      monstro_end_group(ctx);

      monstro_separator_text(ctx, "Tabs");

      if (monstro_begin_tab_bar(ctx, "demoTabs", MONSTRO_TAB_BAR_NONE))
      {
         if (monstro_begin_tab_item(ctx, "General", nullptr))
         {
            monstro_label(ctx, "General settings go here.");
            if (monstro_button(ctx, "Reset Counter", nullptr))
            {
               d->buttonClicks = 0;
            }
            monstro_end_tab_item(ctx);
         }
         if (monstro_begin_tab_item(ctx, "Advanced", nullptr))
         {
            monstro_label(ctx, "Advanced options.");
            monstro_checkbox(ctx, "Extra Feature", &d->checkboxValue, nullptr);
            monstro_end_tab_item(ctx);
         }
         if (monstro_begin_tab_item(ctx, "About", nullptr))
         {
            monstro_label(ctx, "Monstro UI v2.0");
            monstro_end_tab_item(ctx);
         }
         monstro_end_tab_bar(ctx);
      }

      monstro_separator_text(ctx, "Table");

      if (monstro_begin_table(
              ctx, "demoTable", 3,
              (MonstroTableFlags)(MONSTRO_TABLE_BORDERS | MONSTRO_TABLE_ROW_BACKGROUND | MONSTRO_TABLE_RESIZABLE), 0.0f,
              0.0f))
      {
         monstro_table_setup_column(ctx, "Name", MONSTRO_TABLE_COLUMN_NONE, 0.0f);
         monstro_table_setup_column(ctx, "Value", MONSTRO_TABLE_COLUMN_NONE, 0.0f);
         monstro_table_setup_column(ctx, "Action", MONSTRO_TABLE_COLUMN_WIDTH_FIXED, 80.0f);
         monstro_table_headers_row(ctx);

         MonstroStyleButton btn_s = *monstro_get_button_style(ctx);
         btn_s.padding[0] = 0.0f;
         btn_s.padding[1] = 0.0f;
         monstro_push_button_style(ctx, &btn_s);

         monstro_table_next_row(ctx);
         monstro_table_next_column(ctx);
         monstro_label(ctx, "Alpha");
         monstro_table_next_column(ctx);
         monstro_label(ctx, "1.00");
         monstro_table_next_column(ctx);
         if (monstro_button(ctx, "a##Reset", nullptr))
         {
            printf("Reset Alpha\n");
         }

         monstro_table_next_row(ctx);
         monstro_table_next_column(ctx);
         monstro_label(ctx, "Beta");
         monstro_table_next_column(ctx);
         monstro_label(ctx, "2.50");
         monstro_table_next_column(ctx);
         if (monstro_button(ctx, "b##Reset", nullptr))
         {
            printf("Reset Beta\n");
         }

         monstro_table_next_row(ctx);
         monstro_table_next_column(ctx);
         monstro_label(ctx, "Gamma");
         monstro_table_next_column(ctx);
         monstro_label(ctx, "0.75");
         monstro_table_next_column(ctx);
         if (monstro_button(ctx, "g##Reset", nullptr))
         {
            printf("Reset Gamma\n");
         }

         monstro_pop_style(ctx, 1);

         monstro_end_table(ctx);
      }

      monstro_separator_text(ctx, "Editable Table");

      if (monstro_begin_table(
              ctx, "editTable", 3,
              (MonstroTableFlags)(MONSTRO_TABLE_BORDERS | MONSTRO_TABLE_ROW_BACKGROUND | MONSTRO_TABLE_RESIZABLE), 0.0f,
              0.0f))
      {
         monstro_table_setup_column(ctx, "Name", MONSTRO_TABLE_COLUMN_NONE, 0.0f);
         monstro_table_setup_column(ctx, "Value", MONSTRO_TABLE_COLUMN_NONE, 0.0f);
         monstro_table_setup_column(ctx, "Action", MONSTRO_TABLE_COLUMN_WIDTH_FIXED, 80.0f);
         monstro_table_headers_row(ctx);

         MonstroStyleButton btn_s2 = *monstro_get_button_style(ctx);
         btn_s2.padding[0] = 0.0f;
         btn_s2.padding[1] = 0.0f;
         monstro_push_button_style(ctx, &btn_s2);

         MonstroStyleEdit edit_s = *monstro_get_edit_style(ctx);
         edit_s.padding[0] = 0.0f;
         edit_s.padding[1] = 0.0f;
         monstro_push_edit_style(ctx, &edit_s);

         MonstroStyleProperty prop_s = *monstro_get_property_style(ctx);
         prop_s.padding[0] = 0.0f;
         prop_s.padding[1] = 0.0f;
         monstro_push_property_style(ctx, &prop_s);

         MonstroStyleText text_s = *monstro_get_text_style(ctx);
         text_s.alignment = MONSTRO_ALIGN_CENTER;
         monstro_push_text_style(ctx, &text_s);

         const char *row_ids[] = {"name0##", "name1##", "name2##"};
         const char *val_ids[] = {"val0##", "val1##", "val2##"};
         const char *btn_ids[] = {"rst0##Reset", "rst1##Reset", "rst2##Reset"};
         float defaults[] = {1.00f, 2.50f, 0.75f};

         for (int i = 0; i < 3; i++)
         {
            monstro_table_next_row(ctx);
            monstro_table_next_column(ctx);
            monstro_input_text(ctx, row_ids[i], d->tableNames[i], sizeof(d->tableNames[i]), MONSTRO_INPUT_TEXT_NONE,
                               nullptr);
            monstro_table_next_column(ctx);
            monstro_input_float(ctx, val_ids[i], &d->tableValues[i], 0.0f, 0.0f, "%.2f", nullptr);
            monstro_table_next_column(ctx);
            if (monstro_button(ctx, btn_ids[i], nullptr))
            {
               d->tableValues[i] = defaults[i];
            }
         }

         monstro_pop_style(ctx, 4);

         monstro_end_table(ctx);
      }

      monstro_separator_text(ctx, "Label Variants");

      monstro_label_sized(ctx, "Sized Label", 200.0f, 30.0f);
      monstro_label_wrapped(ctx, "This is a long label that will wrap to multiple lines when the width is limited. It demonstrates the monstro_label_wrapped function.", 0.0f); monstro_label_colored(ctx, " Colored Label(Red) ",{255, 80, 80, 255});
      monstro_label_colored(ctx, "Colored Label (Green)", {80, 255, 80, 255});

      monstro_separator_text(ctx, "Spacing Helpers");

      monstro_label(ctx, "horizontal layout demo:");
      monstro_begin_layout(ctx, MONSTRO_LAYOUT_HORIZONTAL);
      {
         if (monstro_button(ctx, "One", nullptr))
         {
         }
         if (monstro_button(ctx, "Two", nullptr))
         {
         }
         if (monstro_button(ctx, "Three", nullptr))
         {
         }
      }
      monstro_end_layout(ctx);

      monstro_label(ctx, "dummy spacer (50x20):");
      monstro_dummy(ctx, 50.0f, 20.0f);
      monstro_label(ctx, "(after spacer)");

      monstro_separator_text(ctx, "Child Scroll Region");

      if (monstro_begin_child(ctx, "scrollList", 0.0f, 120.0f,
                              (MonstroWindowFlags)(MONSTRO_WINDOW_BORDERS | MONSTRO_WINDOW_BACKGROUND)))
      {
         for (int i = 0; i < 30; i++)
         {
            char buf[64];
            snprintf(buf, sizeof(buf), "Item %d", i);
            monstro_label(ctx, buf);
         }
         monstro_end_child(ctx);
      }

      monstro_separator_text(ctx, "Disabled State");

      monstro_begin_disabled(ctx, true);
      monstro_button(ctx, "Disabled Button", nullptr);
      monstro_checkbox(ctx, "Disabled Checkbox", &d->checkboxValue, nullptr);
      monstro_slider_float(ctx, "Disabled Slider", &d->sliderValue, 0.0f, 1.0f, "%.2f", nullptr);
      monstro_end_disabled(ctx);

      monstro_separator_text(ctx, "Item State Queries");

      if (monstro_button(ctx, "Hover/Click Me##statequery", nullptr))
      {
         printf("Item state: clicked!\n");
      }
      {
         bool h = monstro_is_item_hovered(ctx);
         bool a = monstro_is_item_active(ctx);
         MonstroRect r = monstro_get_item_rect(ctx);
         char state_buf[128];
         snprintf(state_buf, sizeof(state_buf), "Hovered: %s  Active: %s  Rect: %.0f,%.0f %.0fx%.0f", h ? "yes" : "no",
                  a ? "yes" : "no", r.x, r.y, r.w, r.h);
         monstro_label(ctx, state_buf);
      }

      monstro_separator_text(ctx, "File Dialog");
      monstro_begin_layout(ctx, MONSTRO_LAYOUT_HORIZONTAL);
      {
          if (monstro_button(ctx, "Open File...", nullptr)) {
              monstro_file_dialog_open(d->fileDialog, "Open File", nullptr,
         "Source Files|*.cpp;*.h;*.txt|All Files|*",
         MONSTRO_FILE_DIALOG_OPEN);
          }
          if (monstro_button(ctx, "Save File...", nullptr)) {
              monstro_file_dialog_open(d->fileDialog, "Save File", nullptr,
         "Text Files|*.txt|All Files|*", MONSTRO_FILE_DIALOG_SAVE);
          }
      }
      monstro_end_layout(ctx);
      if (monstro_file_dialog_is_open(d->fileDialog)) {
          MonstroFileDialogResult r = monstro_file_dialog_draw(ctx, d->fileDialog);
          if (r == MONSTRO_FILE_DIALOG_OK) {
              strncpy(d->lastFilePath, monstro_file_dialog_get_path(d->fileDialog), sizeof(d->lastFilePath) - 1);
              printf("Selected: %s\n", d->lastFilePath);
          }
      }
      if (d->lastFilePath[0]) {
          monstro_label(ctx, d->lastFilePath);
      }

      monstro_separator_text(ctx, "Color Picker");
      if (monstro_color_button(ctx, "##picker_swatch", d->pickerColor, 40.0f, 20.0f)) {
          d->colorPickerOpen = !d->colorPickerOpen;
      }

      monstro_separator_text(ctx, "Modal Dialog");

      if (monstro_button(ctx, "Open Modal", nullptr)) {
          d->showModal = true;
          monstro_open_popup_ex(ctx, "ConfirmModal", true);
      }

      if (d->showModal) {
          if (monstro_begin_popup_modal(ctx, "ConfirmModal", &d->showModal, (MonstroWindowFlags)0)) {
              monstro_label(ctx, "Are you sure?");
              monstro_label(ctx, "This action cannot be undone.");
              monstro_spacer(ctx, 10.0f);
              monstro_begin_layout(ctx, MONSTRO_LAYOUT_HORIZONTAL);
              {
                  if (monstro_button(ctx, "OK", nullptr)) {
                      printf("Modal: OK clicked\n");
                      d->showModal = false;
                      monstro_close_current_popup(ctx);
                  }
                  monstro_spacer(ctx, 10.0f);
                  if (monstro_button(ctx, "Cancel", nullptr)) {
                      printf("Modal: Cancel clicked\n");
                      d->showModal = false;
                      monstro_close_current_popup(ctx);
                  }
              }
              monstro_end_layout(ctx);
              monstro_end_popup(ctx);
          }
      }
      monstro_end_window(ctx);
   }

   if (d->colorPickerOpen)
   {
      monstro_set_next_window_pos(ctx, 550.0f, 300.0f, MONSTRO_COND_FIRST_USE_EVER);
      monstro_set_next_window_size(ctx, 250.0f, 400.0f, MONSTRO_COND_FIRST_USE_EVER);

      MonstroWindowFlags pickerFlags = (MonstroWindowFlags)(
          MONSTRO_WINDOW_BACKGROUND | MONSTRO_WINDOW_TITLE_BAR | MONSTRO_WINDOW_BORDERS | MONSTRO_WINDOW_MOVABLE |
          MONSTRO_WINDOW_COLLAPSABLE | MONSTRO_WINDOW_RESIZABLE | MONSTRO_WINDOW_INPUTS |
          MONSTRO_WINDOW_SCROLL_WITH_MOUSE);

      if (monstro_begin_window(ctx, "Color Picker", &d->colorPickerOpen, pickerFlags))
      {
         monstro_color_picker(ctx, "##demo_picker", &d->pickerColor, MONSTRO_COLOR_PICKER_NONE);
      }
      monstro_end_window(ctx);
   }
}

static void basic_shutdown(ExampleApp *app, void *ud)
{
   BasicDemo *d = (BasicDemo *)ud;
   if (d->fileDialog)
   {
      monstro_file_dialog_destroy(d->fileDialog);
      d->fileDialog = nullptr;
   }
   if (d->iconAtlasTexture != VE_INVALID_TEXTURE)
   {
      veDestroyTexture(app->device, d->iconAtlasTexture);
   }
}

int main(int argc, char *argv[])
{
   (void)argc;
   (void)argv;

   BasicDemo demo = {};
   strncpy(demo.tableNames[0], "Alpha", sizeof(demo.tableNames[0]));
   strncpy(demo.tableNames[1], "Beta", sizeof(demo.tableNames[1]));
   strncpy(demo.tableNames[2], "Gamma", sizeof(demo.tableNames[2]));
   demo.tableValues[0] = 1.00f;
   demo.tableValues[1] = 2.50f;
   demo.tableValues[2] = 0.75f;
   demo.secondWindowOpen = true;
   demo.pickerColor = {0.4f, 0.7f, 1.0f, 1.0f};
   demo.fileDialog = monstro_file_dialog_create();
   demo.iconAtlasTexture = VE_INVALID_TEXTURE;

   ExampleAppDesc desc = {};
   desc.title = "Monstro UI Example";
   desc.width = 1280;
   desc.height = 1024;
   desc.user_data = &demo;
   desc.init = basic_init;
   desc.update = basic_update;
   desc.shutdown = basic_shutdown;

   return example_app_run(desc);
}

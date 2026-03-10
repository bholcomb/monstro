#ifndef MONSTRO_INTERNAL_CONTEXT_H
#define MONSTRO_INTERNAL_CONTEXT_H

#include "monstro.h"
#include "constants.h"
#include "window.h"
#include "popup.h"
#include "input.h"
#include "drawbuffer.h"
#include "font.h"
#include "helpers.h"
#include "idStack.h"
#include "tooltip.h"
#include "editstate.h"
#include <unordered_map>

enum MonstroStylePushedType {
    MONSTRO_PUSHED_TEXT,
    MONSTRO_PUSHED_BUTTON,
    MONSTRO_PUSHED_CONTEXTUAL_BUTTON,
    MONSTRO_PUSHED_MENU_BUTTON,
    MONSTRO_PUSHED_OPTION,
    MONSTRO_PUSHED_CHECKBOX,
    MONSTRO_PUSHED_SELECTABLE,
    MONSTRO_PUSHED_SLIDER,
    MONSTRO_PUSHED_KNOB,
    MONSTRO_PUSHED_PROGRESS,
    MONSTRO_PUSHED_PROPERTY,
    MONSTRO_PUSHED_EDIT,
    MONSTRO_PUSHED_TREE,
    MONSTRO_PUSHED_TAB,
    MONSTRO_PUSHED_TABLE,
    MONSTRO_PUSHED_COMBO,
    MONSTRO_PUSHED_MENU,
    MONSTRO_PUSHED_SEPARATOR,
    MONSTRO_PUSHED_TOOLTIP,
    MONSTRO_PUSHED_CHART,
    MONSTRO_PUSHED_SCROLLH,
    MONSTRO_PUSHED_SCROLLV,
    MONSTRO_PUSHED_WINDOW
};

struct MonstroStyleBackup {
    MonstroStylePushedType type;
    union {
        MonstroStyleText text;
        MonstroStyleButton button;
        MonstroStyleToggle toggle;
        MonstroStyleSelectable selectable;
        MonstroStyleSlider slider;
        MonstroStyleKnob knob;
        MonstroStyleProgress progress;
        MonstroStyleProperty property;
        MonstroStyleEdit edit;
        MonstroStyleTree tree;
        MonstroStyleTab tab;
        MonstroStyleTable table;
        MonstroStyleCombo combo;
        MonstroStyleMenu menu;
        MonstroStyleSeparator separator;
        MonstroStyleTooltip tooltip;
        MonstroStyleChart chart;
        MonstroStyleScrollbar scrollbar;
        MonstroStyleWindow window;
    } data;
};

struct MonstroTreeNodeState {
    uint64_t id = 0;
    bool open = false;
    int last_frame_active = 0;
};

struct MonstroTableColumn {
    char label[128] = {};
    MonstroTableColumnFlags flags = MONSTRO_TABLE_COLUMN_NONE;
    float width = 0.0f;
    float init_width = 0.0f;
};

struct MonstroTableState {
    uint64_t id = 0;
    MonstroTableFlags flags = MONSTRO_TABLE_NONE;
    int column_count = 0;
    int setup_column_count = 0;
    int current_column = -1;
    int current_row = -1;
    MonstroTableColumn columns[MONSTRO_MAX_TABLE_COLUMNS] = {};

    float table_x = 0.0f, table_y = 0.0f;
    float table_w = 0.0f;
    float row_y = 0.0f;
    int last_frame_active = 0;
};

struct MonstroColumnsState {
    int column_count = 0;
    int current_column = 0;
    float columns_x = 0.0f;
    float start_y = 0.0f;
    float row_y = 0.0f;
    float columns_w = 0.0f;
    float row_max_h = 0.0f;
    float widths[MONSTRO_MAX_COLUMNS] = {};
    float offsets[MONSTRO_MAX_COLUMNS] = {};
};

struct MonstroTabBarState {
    uint64_t id;
    uint64_t selected_tab;
    int last_frame_active;
    float header_start_x;
    float header_start_y;
    float header_height;
    bool content_active;
};

struct MonstroIconEntry {
    uint64_t texture_id = 0;
    MonstroVec2 uv_min = {0.0f, 0.0f};
    MonstroVec2 uv_max = {1.0f, 1.0f};
    bool is_monochrome = false;
};

namespace monstro {

struct Context {
    
    
    Context();
    ~Context();

    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;

    void beginFrame(float deltaTime);
    void endFrame();

    void setDefaultFont(MonstroFont* font);
    MonstroFont* getDefaultFont() const { return style.font; }

    void setActiveId(uint64_t id);
    void keepAliveId(uint64_t id);

    Window* findWindow(const char* name);
    Window* createWindow(const char* name, MonstroWindowFlags flags);
    Window* findHoveredWindow(float x, float y, bool excludeChildren = false);
    void focusWindow(Window* window);
    void pushWindow(Window* win);
    void popWindow();

    Layout* getCurrentLayout();



    
    DrawBuffer draw_buffer;

    MonstroDrawCommand final_commands[MONSTRO_MAX_DRAW_COMMANDS];
    uint32_t final_command_count = 0;

    MonstroFontManager* font_manager = nullptr;

    std::unordered_map<uint32_t, MonstroIconEntry> icon_map;

    IdStack id_stack;

    MonstroStyle style = {};
    MonstroStyleBackup style_stack[MONSTRO_MAX_STYLE_VAR_STACK] = {};
    int style_stack_top = 0;

    Window windows[MONSTRO_MAX_WINDOWS] = {};
    int window_count = 0;
    Window* root_window = nullptr;
    Window* current_window = nullptr;
    Window* hovered_window = nullptr;
    Window* focused_window = nullptr;

    Window* window_stack[MONSTRO_MAX_WINDOWS] = {};
    int window_stack_top = 0;

    MonstroPopupInternal popup_stack[MONSTRO_MAX_POPUP_STACK] = {};
    int popup_stack_top = 0;

    MonstroTabBarState tab_bars[MONSTRO_MAX_TAB_BARS] = {};
    int tab_bar_count = 0;
    MonstroTabBarState* current_tab_bar = nullptr;

    MonstroTreeNodeState tree_states[MONSTRO_MAX_TREE_STATES] = {};
    int tree_state_count = 0;
    MonstroTreeNodeState* findOrCreateTreeState(uint64_t id, bool default_open);

    MonstroTableState tables[MONSTRO_MAX_TABLES] = {};
    int table_count = 0;
    MonstroTableState* current_table = nullptr;

    MonstroColumnsState columns_stack[MONSTRO_MAX_COLUMN_STACK] = {};
    int columns_stack_top = 0;

    uint64_t hovered_id = 0;
    uint64_t hovered_id_prev_frame = 0;
    uint64_t active_id = 0;
    uint64_t active_id_prev_frame = 0;
    bool active_id_is_alive = false;

    uint64_t last_item_id = 0;
    MonstroRect last_item_rect = {};
    bool last_item_hovered = false;
    bool last_item_active = false;
    bool last_item_clicked = false;
    bool last_item_edited = false;
    bool last_item_deactivated_after_edit = false;

    bool disabled = false;
    int disabled_stack_depth = 0;

    MouseState mouse;
    KeyboardState keyboard;
    MonstroMouseCursor mouse_cursor = MONSTRO_MOUSE_CURSOR_ARROW;

    bool want_capture_mouse = false;
    bool want_capture_keyboard = false;
    bool want_text_input = false;

    float display_width = 0.0f;
    float display_height = 0.0f;
    float delta_time = 0.0f;
    float time = 0.0f;
    int frame = 0;

    float next_window_pos_x = 0.0f, next_window_pos_y = 0.0f;
    float next_window_size_x = 0.0f, next_window_size_y = 0.0f;
    bool next_window_focus = false;
    MonstroCondition next_window_pos_cond = MONSTRO_COND_NONE;
    MonstroCondition next_window_size_cond = MONSTRO_COND_NONE;
    MonstroCondition next_window_focus_cond = MONSTRO_COND_NONE;

    MonstroFont* default_font = nullptr;

    TooltipState tooltip;
    EditState edit_state;
};

} // namespace monstro

typedef monstro::Context MonstroContext;

#endif

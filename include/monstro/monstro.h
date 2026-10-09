#ifndef MONSTRO_MONSTRO_H
#define MONSTRO_MONSTRO_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The environment holds what every UI shares, once: fonts and their atlas,
 * themes, icons, settings, and the canvases frames are built in. It is set up
 * between frames; while any frame is open it is read-only.
 *
 * Capacities are fixed for the life of the environment. A frame or a load that
 * needs more stops the process with a message naming the capacity, its value
 * and what was needed, so a UI that outgrows them gets noticed and fixed.
 * monstro_env_config_default() returns the configuration monstro_init(NULL) uses.
 */
typedef struct MonstroEnvConfig {
    uint32_t canvas_count;      /* canvases frames borrow in turn; each holds the capacities below */
    uint32_t max_vertices;      /* vertices per frame in a canvas's own vertex buffer */
    uint32_t max_indices;       /* indices per frame in a canvas's own index buffer */
    uint32_t max_draw_commands; /* draw commands per frame */
    uint32_t max_path_points;   /* points in one canvas path */
    uint32_t atlas_width;       /* the font atlas, in pixels */
    uint32_t atlas_height;
    uint32_t max_fonts;
    uint32_t max_themes;        /* including the three built-in themes */
    uint32_t max_icons;
    MonstroVertexLayout vertex_layout; /* placement of vertices in host buffers (MonstroFrameBuffers) */
} MonstroEnvConfig;

/* Global settings, shared by every context of an environment. */
typedef struct MonstroSettings {
    float double_click_time;     /* seconds between the clicks of a double click */
    float double_click_distance; /* pixels the mouse may move between them */
    float drag_threshold;        /* pixels before a press becomes a drag */
    float key_repeat_delay;      /* seconds before a held key or button repeats */
    float key_repeat_rate;       /* seconds between repeats */
    float tooltip_delay;         /* seconds the mouse rests before a tooltip shows */
} MonstroSettings;

/*
 * Per-UI capacities: the state a context keeps between frames. A context
 * holds one UI (a screen, a panel, a tool window) with its own input, display
 * size and interaction state.
 */
typedef struct MonstroContextConfig {
    uint32_t max_windows;    /* windows, child regions, popups and menus ever begun, the root included */
    uint32_t max_tree_nodes; /* tree nodes and collapsing headers with remembered state */
    uint32_t max_tab_bars;
    uint32_t max_tables;
    uint32_t max_popups; /* popups open at once */
    uint32_t max_input_events; /* input events waiting for a frame; see the input setters in input.h */
} MonstroContextConfig;

/* ------------------------------------------------------------------------- */
/* Environment                                                               */
/* ------------------------------------------------------------------------- */

MonstroEnvConfig monstro_env_config_default(void);

/* Creates an environment; config may be NULL for the defaults. Returns NULL only when memory runs out. */
MonstroEnv* monstro_init(const MonstroEnvConfig* config);
/* Destroys the environment with its fonts, themes and canvases. Destroy its contexts first. */
void monstro_shutdown(MonstroEnv* env);

MonstroEnvConfig monstro_env_get_config(MonstroEnv* env);

MonstroSettings monstro_env_get_settings(MonstroEnv* env);
void monstro_env_set_settings(MonstroEnv* env, const MonstroSettings* settings);

/* ------------------------------------------------------------------------- */
/* Context                                                                   */
/* ------------------------------------------------------------------------- */

MonstroContextConfig monstro_context_config_default(void);

/* Creates a context on env; config may be NULL for the defaults. Returns NULL only when memory runs out. */
MonstroContext* monstro_context_create(MonstroEnv* env, const MonstroContextConfig* config);
void monstro_context_destroy(MonstroContext* ctx);

MonstroEnv* monstro_context_get_env(MonstroContext* ctx);

void monstro_set_display_size(MonstroContext* ctx, float width, float height);
void monstro_get_display_size(MonstroContext* ctx, float* out_width, float* out_height);

/* ------------------------------------------------------------------------- */
/* Frames                                                                    */
/* ------------------------------------------------------------------------- */

/*
 * begin_frame borrows a canvas from the environment for this context's frame;
 * end_frame returns it and yields the frame's draw data, which stays valid
 * until that canvas is lent again. The canvases are lent in turn, so with
 * canvas_count N the draw data of the last N frames stays valid.
 */
void monstro_begin_frame(MonstroContext* ctx, float delta_time);
/* Builds the frame into host storage; see MonstroFrameBuffers. The buffers are used for this frame only. */
void monstro_begin_frame_into(MonstroContext* ctx, float delta_time, const MonstroFrameBuffers* buffers);
const MonstroDrawData* monstro_end_frame(MonstroContext* ctx);

float monstro_get_delta_time(MonstroContext* ctx);
float monstro_get_time(MonstroContext* ctx);

void monstro_push_id(MonstroContext* ctx, const char* name);
void monstro_pop_id(MonstroContext* ctx);
uint64_t monstro_get_id(MonstroContext* ctx, const char* name);

#ifdef __cplusplus
}
#endif

#endif

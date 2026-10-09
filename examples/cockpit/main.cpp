/**
 * A cockpit screen: one 512 x 512 multi-function display, drawn as a game
 * draws an MFD on a render target. The flight page has an artificial horizon
 * (sky and ground rolled and pitched, a pitch ladder with turned numbers, the
 * aircraft symbol), two dials, four vertical rotor bars, an absolute knob and
 * a vertical lift lever; the systems and cargo pages have gauges, lit buttons
 * and gear lamps. The page tabs light the page the "simulation" keeps: a click
 * asks for a page, and the page changes when the simulation takes the request.
 */
#include "../common/example_app.h"
#include <math.h>
#include <stdio.h>

static const float PI = 3.14159265f;

/* -- colours: plain 8-bit RGBA, packed as monstro draws them -------------- */

static uint32_t rgb(uint32_t r, uint32_t g, uint32_t b) { return 0xff000000u | b << 16 | g << 8 | r; }

static const uint32_t SCREEN = rgb(8, 12, 10);
static const uint32_t OFF = rgb(24, 32, 28);
static const uint32_t NORMAL = rgb(200, 220, 205);
static const uint32_t ACTIVE = rgb(70, 220, 110);
static const uint32_t CAUTION = rgb(240, 180, 40);
static const uint32_t WARNING = rgb(240, 70, 50);
static const uint32_t SKY = rgb(40, 110, 190);
static const uint32_t GROUND = rgb(120, 80, 40);

static MonstroColor colour(uint32_t packed) { return monstro_color_from_u32(packed); }

/* -- the simulation's side: values and the page --------------------------- */

struct Ship
{
   float time;
   float pitch_deg, roll_rad, speed, altitude, climb;
   float rotor[4];
   float lift;        // set by the lever
   float heading_bug; // set by the knob
   float battery, heat;
   bool main_power, ramp_open, clamp;
   int page;           // the page the simulation keeps
   int requested_page; // a page asked for by a click, taken on the next tick
};

static Ship g_ship = {};
static MonstroFont *g_small = nullptr;
static MonstroFont *g_large = nullptr;

static void simulate(Ship *s, float dt)
{
   s->time += dt;
   if (s->requested_page >= 0)
   {
      s->page = s->requested_page;
      s->requested_page = -1;
   }
   s->pitch_deg = 12.0f * sinf(s->time * 0.4f);
   s->roll_rad = 0.45f * sinf(s->time * 0.27f);
   s->climb = 6.0f * (s->lift - 0.5f) + sinf(s->time) * 0.4f;
   s->altitude = fmaxf(0.0f, 10.0f + 8.0f * sinf(s->time * 0.1f) + s->climb);
   s->speed = 30.0f + 25.0f * s->lift;
   for (int i = 0; i < 4; i++)
      s->rotor[i] = fminf(1.0f, fmaxf(0.0f, s->lift + 0.08f * sinf(s->time * 1.3f + (float)i)));
   s->battery = 0.5f + 0.45f * sinf(s->time * 0.05f);
   s->heat = 0.55f + 0.4f * sinf(s->time * 0.21f);
}

/* -- drawing helpers ------------------------------------------------------ */

static MonstroVec2 vec(float x, float y) { return {x, y}; }

// The tone a value shows in: warning, then caution, beyond their limits.
static uint32_t tone(float value, float caution_above, float warning_above)
{
   return value > warning_above ? WARNING : (value > caution_above ? CAUTION : ACTIVE);
}

static void readout(MonstroCanvas *canvas, MonstroRect at, const char *label, const char *value, uint32_t frame)
{
   monstro_draw_rect_filled(canvas, {at.x + 4, at.y + 4, at.w - 8, at.h - 8}, OFF, 6, MONSTRO_CORNER_ALL);
   monstro_draw_rect(canvas, {at.x + 4, at.y + 4, at.w - 8, at.h - 8}, frame, 6, MONSTRO_CORNER_ALL, 2);
   monstro_draw_text_aligned(canvas, g_small, {at.x + 12, at.y, 60, at.h}, NORMAL, label,
                             (MonstroAlign)(MONSTRO_ALIGN_LEFT | MONSTRO_ALIGN_VCENTER));
   monstro_draw_text_aligned(canvas, g_large, {at.x + 60, at.y, at.w - 72, at.h}, NORMAL, value,
                             (MonstroAlign)(MONSTRO_ALIGN_RIGHT | MONSTRO_ALIGN_VCENTER));
}

// The artificial horizon: a clipped box with the sky and ground rolled about its centre and moved by pitch.
static void horizon(MonstroCanvas *canvas, MonstroRect at, float pitch_deg, float roll_rad)
{
   const float pixels_per_degree = 4.0f;
   MonstroVec2 centre = {at.x + at.w * 0.5f, at.y + at.h * 0.5f};
   float angle = -roll_rad; // the world turns against the roll
   float c = cosf(angle), s = sinf(angle);
   float shift = pitch_deg * pixels_per_degree;
   // A point in the horizon's own frame (x along it, y down from it), on the screen.
   auto place = [&](float x, float y)
   {
      y += shift;
      return vec(centre.x + x * c - y * s, centre.y + x * s + y * c);
   };

   monstro_push_clip_rect(canvas, at);
   const float far = 600.0f;
   monstro_draw_quad_filled(canvas, place(-far, -far), place(far, -far), place(far, 0), place(-far, 0), SKY);
   monstro_draw_quad_filled(canvas, place(-far, 0), place(far, 0), place(far, far), place(-far, far), GROUND);
   monstro_draw_line(canvas, place(-far, 0), place(far, 0), NORMAL, 2);

   // The pitch ladder: a rung every 10 degrees, its number turned with it.
   for (int deg = -30; deg <= 30; deg += 10)
   {
      if (deg == 0)
         continue;
      float y = -deg * pixels_per_degree;
      float half = deg % 20 == 0 ? 50.0f : 34.0f;
      monstro_draw_line(canvas, place(-half, y), place(half, y), NORMAL, 2);
      char number[8];
      snprintf(number, sizeof(number), "%d", deg < 0 ? -deg : deg);
      // Laid out level beside the rung, then turned with the horizon about the screen point it rolls about.
      MonstroVec2 rung_end = {centre.x + half + 6, centre.y + y + shift};
      monstro_draw_text_rotated(canvas, g_small, {rung_end.x, rung_end.y - 10, 30, 20}, NORMAL, number,
                                (MonstroAlign)(MONSTRO_ALIGN_LEFT | MONSTRO_ALIGN_VCENTER), centre, angle);
      monstro_draw_text_rotated(canvas, g_small, {centre.x - half - 36, rung_end.y - 10, 30, 20}, NORMAL, number,
                                (MonstroAlign)(MONSTRO_ALIGN_RIGHT | MONSTRO_ALIGN_VCENTER), centre, angle);
   }

   // The roll scale above, fixed, and its pointer, turned.
   monstro_path_clear(canvas);
   monstro_path_arc_to(canvas, centre, 130, -PI * 0.5f - 0.8f, -PI * 0.5f + 0.8f, 24);
   monstro_path_stroke(canvas, NORMAL, false, 2);
   MonstroVec2 tip = place(0, -126 - shift);
   MonstroVec2 left = place(-7, -112 - shift), right = place(7, -112 - shift);
   monstro_draw_triangle_filled(canvas, tip, right, left, CAUTION);
   monstro_pop_clip_rect(canvas);

   // The aircraft symbol, fixed: wings and a dot.
   monstro_draw_line(canvas, {centre.x - 70, centre.y}, {centre.x - 22, centre.y}, CAUTION, 4);
   monstro_draw_line(canvas, {centre.x + 22, centre.y}, {centre.x + 70, centre.y}, CAUTION, 4);
   monstro_draw_ellipse_filled(canvas, centre, {6, 6}, 0, CAUTION, 16);
   monstro_draw_rect(canvas, at, NORMAL, 0, MONSTRO_CORNER_NONE, 1);
}

// A dial: a 270 degree scale with a caution band, a needle and the value below.
static void dial(MonstroCanvas *canvas, MonstroRect at, const char *label, float value, float minimum, float maximum,
                 float caution_above, const char *text)
{
   const float from = PI * 0.75f, to = PI * 2.25f;
   MonstroVec2 centre = {at.x + at.w * 0.5f, at.y + at.h * 0.56f};
   float radius = at.w * 0.36f;
   float band = from + (to - from) * (caution_above - minimum) / (maximum - minimum);
   monstro_path_clear(canvas);
   monstro_path_arc_to(canvas, centre, radius, from, band, 32);
   monstro_path_stroke(canvas, NORMAL, false, 3);
   monstro_path_clear(canvas);
   monstro_path_arc_to(canvas, centre, radius, band, to, 12);
   monstro_path_stroke(canvas, CAUTION, false, 5);
   float t = fminf(1.0f, fmaxf(0.0f, (value - minimum) / (maximum - minimum)));
   float a = from + (to - from) * t;
   monstro_draw_line(canvas, centre, {centre.x + cosf(a) * radius * 0.9f, centre.y + sinf(a) * radius * 0.9f},
                     value > caution_above ? CAUTION : ACTIVE, 3);
   monstro_draw_circle_filled(canvas, centre, 4, NORMAL, 12);
   monstro_draw_text_aligned(canvas, g_small, {at.x, at.y + 2, at.w, 20}, NORMAL, label, MONSTRO_ALIGN_CENTER);
   monstro_draw_text_aligned(canvas, g_small, {at.x, at.y + at.h - 24, at.w, 20}, NORMAL, text, MONSTRO_ALIGN_CENTER);
}

// A vertical gauge with no text of its own, its fill toned by its value, labelled below.
static void vertical_gauge(MonstroContext *ctx, MonstroRect at, const char *label, float value, uint32_t fill)
{
   MonstroStyleProgress style = *monstro_get_progress_style(ctx);
   style.normal.data.color = colour(OFF);
   style.cursor_normal.data.color = colour(fill);
   style.rounding = style.cursor_rounding = 3;
   style.padding[0] = style.padding[1] = 0;
   monstro_push_progress_style(ctx, &style);
   monstro_set_cursor_screen_pos(ctx, at.x + (at.w - 22) * 0.5f, at.y + 4);
   monstro_progress_bar_ex(ctx, value, 22, at.h - 30, "", MONSTRO_PROGRESS_VERTICAL);
   monstro_pop_style(ctx, 1);
   MonstroCanvas *canvas = monstro_get_window_canvas(ctx);
   monstro_draw_text_aligned(canvas, g_small, {at.x, at.y + at.h - 24, at.w, 20}, NORMAL, label, MONSTRO_ALIGN_CENTER);
}

// A lit button: a sized selectable with its text centred, lit while on.
static bool lit_button(MonstroContext *ctx, const char *label, bool on, MonstroRect at)
{
   MonstroStyleSelectable style = *monstro_get_selectable_style(ctx);
   style.text_alignment = MONSTRO_ALIGN_CENTER;
   style.padding[0] = style.padding[1] = 0;
   style.normal = style.hover = style.pressed = {MONSTRO_STYLE_ITEM_COLOR, {colour(OFF)}};
   style.normal_active = style.hover_active = style.pressed_active = {MONSTRO_STYLE_ITEM_COLOR, {colour(ACTIVE)}};
   style.text_normal = style.text_hover = style.text_pressed = colour(NORMAL);
   style.text_normal_active = style.text_hover_active = style.text_pressed_active = colour(SCREEN);
   monstro_push_selectable_style(ctx, &style);
   monstro_set_cursor_screen_pos(ctx, at.x + 6, at.y + 6);
   bool lit = on;
   bool pressed = monstro_selectable_sized(ctx, label, &lit, at.w - 12, at.h - 12, MONSTRO_SELECTABLE_NONE, nullptr);
   monstro_pop_style(ctx, 1);
   monstro_draw_rect(monstro_get_window_canvas(ctx), {at.x + 6, at.y + 6, at.w - 12, at.h - 12}, NORMAL, 0,
                     MONSTRO_CORNER_NONE, 1);
   return pressed;
}

// A lamp: an ellipse in its box, lit or dark, with its name.
static void lamp(MonstroCanvas *canvas, MonstroRect at, const char *name, bool lit)
{
   MonstroVec2 centre = {at.x + at.w * 0.5f, at.y + at.h * 0.4f};
   MonstroVec2 radii = {at.w * 0.36f, at.h * 0.24f};
   monstro_draw_ellipse_filled(canvas, centre, radii, 0, lit ? ACTIVE : OFF, 32);
   monstro_draw_ellipse(canvas, centre, radii, 0, NORMAL, 32, 2);
   monstro_draw_text_aligned(canvas, g_small, {at.x, at.y + at.h * 0.72f, at.w, 20}, NORMAL, name,
                             MONSTRO_ALIGN_CENTER);
}

/* -- the pages ------------------------------------------------------------ */

static void flight_page(MonstroContext *ctx, MonstroCanvas *canvas, Ship *s)
{
   horizon(canvas, {0, 0, 320, 320}, s->pitch_deg, s->roll_rad);

   char text[32];
   snprintf(text, sizeof(text), "%.0f km/h", s->speed * 3.6f);
   readout(canvas, {320, 0, 192, 64}, "SPD", text, NORMAL);
   snprintf(text, sizeof(text), "%.1f m", s->altitude);
   readout(canvas, {320, 64, 192, 64}, "AGL", text, s->altitude < 2.0f ? CAUTION : NORMAL);
   snprintf(text, sizeof(text), "%+.1f m/s", s->climb);
   readout(canvas, {320, 128, 192, 64}, "V/S", text, s->climb < -2.0f ? CAUTION : NORMAL);

   // The heading bug: an absolute knob points where it is pressed.
   float bug = s->heading_bug;
   monstro_set_cursor_screen_pos(ctx, 336, 200);
   if (monstro_knob_ex(ctx, "hdg##", &bug, 0.0f, 360.0f, nullptr, 84.0f, MONSTRO_KNOB_ABSOLUTE, nullptr))
      s->heading_bug = bug;
   snprintf(text, sizeof(text), "HDG %03.0f", s->heading_bug);
   monstro_draw_text_aligned(canvas, g_small, {320, 292, 112, 24}, NORMAL, text, MONSTRO_ALIGN_CENTER);

   // The lift lever: a vertical slider.
   float lift = s->lift;
   monstro_set_cursor_screen_pos(ctx, 456, 196);
   if (monstro_slider_float_ex(ctx, "lift##", &lift, 0.0f, 1.0f, nullptr, 28.0f, 96.0f, MONSTRO_SLIDER_VERTICAL,
                               nullptr))
      s->lift = lift;
   monstro_draw_text_aligned(canvas, g_small, {432, 296, 80, 20}, NORMAL, "LIFT", MONSTRO_ALIGN_CENTER);

   snprintf(text, sizeof(text), "%+.1f m/s", s->climb);
   dial(canvas, {0, 320, 128, 128}, "V/S", s->climb, -5.0f, 5.0f, 3.0f, text);
   snprintf(text, sizeof(text), "%.1f m", s->altitude);
   dial(canvas, {128, 320, 128, 128}, "AGL", s->altitude, 0.0f, 20.0f, 16.0f, text);

   const char *rotors[4] = {"R1", "R2", "R3", "R4"};
   for (int i = 0; i < 4; i++)
      vertical_gauge(ctx, {256.0f + 64.0f * (float)i, 320, 64, 128}, rotors[i], s->rotor[i],
                     tone(s->rotor[i], 0.9f, 0.97f));
}

static void systems_page(MonstroContext *ctx, MonstroCanvas *canvas, Ship *s)
{
   vertical_gauge(ctx, {32, 24, 96, 300}, "BATTERY", s->battery, s->battery < 0.2f ? CAUTION : ACTIVE);
   vertical_gauge(ctx, {160, 24, 96, 300}, "HEAT", s->heat, tone(s->heat, 0.7f, 0.9f));
   char text[32];
   snprintf(text, sizeof(text), "%.0f %%", s->battery * 100.0f);
   readout(canvas, {288, 24, 208, 64}, "CHG", text, NORMAL);
   snprintf(text, sizeof(text), "%.0f %%", s->heat * 100.0f);
   readout(canvas, {288, 88, 208, 64}, "HEAT", text, s->heat > 0.7f ? CAUTION : NORMAL);
   if (lit_button(ctx, "main##MAIN POWER", s->main_power, {288, 176, 208, 72}))
      s->main_power = !s->main_power;
}

static void cargo_page(MonstroContext *ctx, MonstroCanvas *canvas, Ship *s)
{
   if (lit_button(ctx, "ramp##RAMP", s->ramp_open, {0, 0, 256, 72}))
      s->ramp_open = !s->ramp_open;
   if (lit_button(ctx, "clamp##CLAMP", s->clamp, {256, 0, 256, 72}))
      s->clamp = !s->clamp;
   // Gear lamps in boxes wider than tall: ellipses.
   const char *gear[4] = {"FL", "FR", "RL", "RR"};
   for (int i = 0; i < 4; i++)
      lamp(canvas, {16.0f + 120.0f * (float)i, 120, 112, 80}, gear[i], s->altitude < 1.0f || i % 2 == 0);
}

/* -- the screen ----------------------------------------------------------- */

static void cockpit_init(ExampleApp *app, void *)
{
   g_small = monstro_font_load_ttf_from_file(app->env, "screen-small", "data/fonts/freeSans.ttf", 16.0f, 32, 96);
   g_large = monstro_font_load_ttf_from_file(app->env, "screen-large", "data/fonts/freeSans.ttf", 26.0f, 32, 96);
   if (!g_small || !g_large)
   {
      fprintf(stderr, "cannot load data/fonts/freeSans.ttf; run the example from bin/\n");
      abort();
   }
   g_ship.lift = 0.5f;
   g_ship.heading_bug = 90.0f;
   g_ship.main_power = true;
   g_ship.requested_page = -1;
}

static void cockpit_update(ExampleApp *app, void *, float dt)
{
   MonstroContext *ctx = app->ui;
   Ship *s = &g_ship;
   simulate(s, dt);

   MonstroStyleWindow window = *monstro_get_window_style(ctx);
   window.fixed_background.data.color = colour(SCREEN);
   monstro_push_window_style(ctx, &window);
   monstro_set_next_window_pos(ctx, 0, 0, MONSTRO_COND_ALWAYS);
   monstro_set_next_window_size(ctx, 512, 512, MONSTRO_COND_ALWAYS);
   monstro_begin_window(ctx, "mfd", nullptr, (MonstroWindowFlags)(MONSTRO_WINDOW_BACKGROUND | MONSTRO_WINDOW_INPUTS));
   MonstroCanvas *canvas = monstro_get_window_canvas(ctx);

   // Every page is drawn clipped to its area above the tabs.
   monstro_push_clip_rect(canvas, {0, 0, 512, 448});
   if (s->page == 0)
      flight_page(ctx, canvas, s);
   else if (s->page == 1)
      systems_page(ctx, canvas, s);
   else
      cargo_page(ctx, canvas, s);
   monstro_pop_clip_rect(canvas);

   // The tabs light the simulation's page; a click asks for another, which the next tick takes.
   const char *pages[3] = {"flt##FLT", "sys##SYS", "cargo##CARGO"};
   int shown = s->page;
   monstro_set_cursor_screen_pos(ctx, 0, 448);
   if (monstro_page_tabs(ctx, "pages", pages, 3, &shown, 512, 64))
      s->requested_page = shown;

   monstro_end_window(ctx);
   monstro_pop_style(ctx, 1);
}

int main(int argc, char *argv[])
{
   (void)argc;
   (void)argv;
   ExampleAppDesc desc = {};
   desc.title = "Monstro Cockpit Example";
   desc.width = 512;
   desc.height = 512;
   desc.init = cockpit_init;
   desc.update = cockpit_update;
   return example_app_run(desc);
}

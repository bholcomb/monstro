#ifndef EXAMPLE_PLOT_H
#define EXAMPLE_PLOT_H

#include <monstro.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define PLOT_MAX_SERIES 8
#define PLOT_MAX_POINTS 1024

typedef enum PlotType
{
   PLOT_LINE,
   PLOT_BAR,
   PLOT_SCATTER
} PlotType;

typedef struct PlotSeries
{
   float x[PLOT_MAX_POINTS];
   float y[PLOT_MAX_POINTS];
   int count;
   uint32_t color;
   char label[64];
   PlotType type;
   float bar_width;
} PlotSeries;

typedef struct PlotWidget
{
   PlotSeries series[PLOT_MAX_SERIES];
   int series_count;

   char title[128];
   char x_label[64];
   char y_label[64];

   float x_min, x_max;
   float y_min, y_max;
   bool auto_fit;

   float width, height;
   bool show_grid;
   bool show_crosshair;
} PlotWidget;

void plot_init(PlotWidget *pw, const char *title, float width, float height);
int plot_add_series(PlotWidget *pw, const char *label, PlotType type, uint32_t color);
void plot_set_data(PlotWidget *pw, int series_idx, const float *x, const float *y, int count);
void plot_set_range(PlotWidget *pw, float x_min, float x_max, float y_min, float y_max);
void plot_draw(PlotWidget *pw, MonstroContext *ctx);

#ifdef __cplusplus
}
#endif

#endif

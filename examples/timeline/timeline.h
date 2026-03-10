#ifndef EXAMPLE_TIMELINE_H
#define EXAMPLE_TIMELINE_H

#include <monstro.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define TIMELINE_MAX_TRACKS 16
#define TIMELINE_MAX_CLIPS 64
#define TIMELINE_MAX_NAME 64

typedef struct TimelineClip
{
   char name[TIMELINE_MAX_NAME];
   float start;
   float duration;
   uint32_t color;
   int track;
} TimelineClip;

typedef struct TimelineTrack
{
   char name[TIMELINE_MAX_NAME];
} TimelineTrack;

typedef struct Timeline
{
   TimelineTrack tracks[TIMELINE_MAX_TRACKS];
   int track_count;

   TimelineClip clips[TIMELINE_MAX_CLIPS];
   int clip_count;

   float playhead;
   float view_start;
   float view_end;
   float total_duration;

   float track_height;
   float header_width;

   int selected_clip;
   int dragging_clip;
   float drag_offset;
} Timeline;

void timeline_init(Timeline *tl, float total_duration);
int timeline_add_track(Timeline *tl, const char *name);
int timeline_add_clip(Timeline *tl, int track, const char *name, float start, float duration, uint32_t color);
void timeline_draw(Timeline *tl, MonstroContext *ctx, float dt);

#ifdef __cplusplus
}
#endif

#endif

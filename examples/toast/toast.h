#ifndef EXAMPLE_TOAST_H
#define EXAMPLE_TOAST_H

#include <monstro.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum ToastType
{
   TOAST_INFO = 0,
   TOAST_SUCCESS = 1,
   TOAST_WARNING = 2,
   TOAST_ERROR = 3
} ToastType;

#define TOAST_MAX_NOTIFICATIONS 16
#define TOAST_MAX_MSG_LEN 256

typedef struct ToastNotification
{
   char message[TOAST_MAX_MSG_LEN];
   ToastType type;
   float duration;
   float elapsed;
   bool active;
} ToastNotification;

typedef struct ToastManager
{
   ToastNotification toasts[TOAST_MAX_NOTIFICATIONS];
} ToastManager;

void toast_init(ToastManager *tm);
void toast_show(ToastManager *tm, ToastType type, float duration, const char *fmt, ...);
void toast_draw(ToastManager *tm, MonstroContext *ctx, float dt);

#ifdef __cplusplus
}
#endif

#endif

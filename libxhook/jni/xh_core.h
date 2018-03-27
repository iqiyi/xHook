#ifndef XH_CORE_H
#define XH_CORE_H 1

#include <android/log.h>

#ifdef __cplusplus
extern "C" {
#endif

#define XH_CORE_EXPORT __attribute__((visibility("default")))

void xh_core_set_log_priority(android_LogPriority priority) XH_CORE_EXPORT;
void xh_core_set_system_hook(int flag) XH_CORE_EXPORT;
void xh_core_set_reldyn_hook(int flag) XH_CORE_EXPORT;

int xh_core_hook(const char *filename, const char *symbol, void *new_func, void **old_func) XH_CORE_EXPORT;
int xh_core_unhook(const char *filename, const char *symbol) XH_CORE_EXPORT;

int xh_core_start() XH_CORE_EXPORT;
int xh_core_stop() XH_CORE_EXPORT;

int xh_core_refresh() XH_CORE_EXPORT;

#ifdef __cplusplus
}
#endif

#endif

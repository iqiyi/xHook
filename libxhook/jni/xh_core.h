#ifndef XH_CORE_H
#define XH_CORE_H 1

#include <android/log.h>

#ifdef __cplusplus
extern "C" {
#endif

void xh_core_set_log_priority(android_LogPriority priority);

int xh_core_hook(const char *filename, const char *symbol, void *new_func, void **old_func);

void xh_core_refresh();

#ifdef __cplusplus
}
#endif

#endif

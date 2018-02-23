#ifndef XH_CORE_H
#define XH_CORE_H 1

#include <android/log.h>

#ifdef __cplusplus
extern "C" {
#endif

void xh_core_set_log_priority(android_LogPriority priority);
void xh_core_set_system_hook(int flag);
void xh_core_set_reldyn_hook(int flag);

int xh_core_hook(const char *filename, const char *symbol, void *new_func, void **old_func);
int xh_core_unhook(const char *filename, const char *symbol);

int xh_core_start();
int xh_core_stop();

int xh_core_refresh();

#ifdef __cplusplus
}
#endif

#endif

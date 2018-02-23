#ifndef XH_MAP_H
#define XH_MAP_H 1

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct xh_map xh_map_t;
typedef int (*xh_map_need_hook_check_t)(const char *pathname, const char *filename, void *arg);

int xh_map_create(xh_map_t **self, int system_hook, int reldyn_hook);
int xh_map_destroy(xh_map_t **self);

int xh_map_refresh(xh_map_t *self);
int xh_map_hook(xh_map_t *self, const char *filename, const char *symbol,
                void *new_func, void **old_func,
                xh_map_need_hook_check_t need_hook_check, void *arg);
void xh_map_hook_finish(xh_map_t *self);

#ifdef __cplusplus
}
#endif

#endif

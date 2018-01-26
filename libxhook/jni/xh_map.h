#ifndef XH_MAP_H
#define XH_MAP_H 1

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct xh_map xh_map_t;

int xh_map_create(xh_map_t **self);
int xh_map_destroy(xh_map_t **self);
void xh_map_dump(xh_map_t *self);

int xh_map_refresh(xh_map_t *self);
int xh_map_hook(xh_map_t *self, const char *filename, const char *symbol, void *new_func, void **old_func);
void xh_map_hook_finish(xh_map_t *self);

#ifdef __cplusplus
}
#endif

#endif

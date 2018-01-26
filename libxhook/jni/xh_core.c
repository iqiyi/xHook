#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <jni.h>
#include "xh_tree.h"
#include "xh_errno.h"
#include "xh_log.h"
#include "xh_map.h"
#include "xh_core.h"
#include "xh_version.h"

#define XH_CORE_DEBUG 0

typedef struct xh_core_stat
{
    const char  *filename;
    const char  *symbol;
    void        *new_func;
    void       **old_func;
    RB_ENTRY(xh_core_stat) link;
} xh_core_stat_t;
static __inline__ int xh_core_stat_cmp(xh_core_stat_t *a, xh_core_stat_t *b)
{
    if(NULL != a->filename && NULL == b->filename)
        return 1;
    else if(NULL == a->filename && NULL != b->filename)
        return -1;
    else if(NULL == a->filename && NULL == b->filename)
        return strcmp(a->symbol, b->symbol);
    else
    {
        int r = strcmp(a->filename, b->filename);
        if(0 != r)
            return r;
        else
            return strcmp(a->symbol, b->symbol);
    }
}
typedef RB_HEAD(xh_core_stat_tree, xh_core_stat) xh_core_stat_tree_t;
RB_GENERATE_STATIC(xh_core_stat_tree, xh_core_stat, link, xh_core_stat_cmp)

static pthread_mutex_t      xh_core_mutex = PTHREAD_MUTEX_INITIALIZER;
static xh_core_stat_tree_t  xh_core_stats = RB_INITIALIZER(NULL);
static xh_map_t            *xh_core_maps  = NULL;

void xh_core_set_log_priority(android_LogPriority priority)
{
    xh_log_priority = priority;
}

static int xh_core_hook_impl(const char *filename, const char *symbol, void *new_func, void **old_func)
{
    xh_core_stat_t  st_key = {.filename = filename, .symbol = symbol};
    xh_core_stat_t *st     = NULL;
    int             r      = 0;

    if(NULL == symbol || 0 == new_func) return XH_ERRNO_INVAL;

    if(NULL != RB_FIND(xh_core_stat_tree, &xh_core_stats, &st_key)) return XH_ERRNO_REPEAT;

    if(NULL == (st = malloc(sizeof(xh_core_stat_t)))) return XH_ERRNO_NOMEM;
    st->filename = NULL;
    if(NULL != filename)
    {
        if(NULL == (st->filename = strdup(filename)))
        {
            r = XH_ERRNO_NOMEM;
            goto err;
        }
    }
    if(NULL == (st->symbol = strdup(symbol)))
    {
        r = XH_ERRNO_NOMEM;
        goto err;
    }
    st->new_func = new_func;
    st->old_func = old_func;

    RB_INSERT(xh_core_stat_tree, &xh_core_stats, st);
    if(old_func) *old_func = NULL;
    return 0;

 err:
    if(NULL != st)
    {
        if(NULL != st->symbol) free((void *)st->symbol);
        if(NULL != st->filename) free((void *)st->filename);
        free(st);
    }
    return r;
}

int xh_core_hook(const char *filename, const char *symbol, void *new_func, void **old_func)
{
    int r;

    if(NULL != xh_core_maps) return XH_ERRNO_PROHIBITED; //already started?
    
    pthread_mutex_lock(&xh_core_mutex);

    r = xh_core_hook_impl(filename, symbol, new_func, old_func);
    
    pthread_mutex_unlock(&xh_core_mutex);

    return r;
}

#if XH_CORE_DEBUG
static void xh_core_dump()
{
    xh_core_stat_t *st = NULL;

    if(xh_log_priority < ANDROID_LOG_DEBUG) return;

    XH_LOG_DEBUG("Stat:\n");
    RB_FOREACH(st, xh_core_stat_tree, &xh_core_stats)
    {
        XH_LOG_DEBUG("  %s : %s : %p -> %p\n",
                     (NULL == st->filename ? "*" : st->filename),
                     st->symbol,
                     (st->old_func ? *(st->old_func) : 0),
                     st->new_func);
    }
}
#endif

static int xh_core_refresh_impl()
{
    xh_core_stat_t *st = NULL;
    int             r  = 0;

    if(0 != (r = xh_map_refresh(xh_core_maps))) return r;

    RB_FOREACH(st, xh_core_stat_tree, &xh_core_stats)
    {
        if(NULL == st->filename)
            xh_map_hook(xh_core_maps, st->filename, st->symbol, st->new_func, st->old_func);
    }
    RB_FOREACH(st, xh_core_stat_tree, &xh_core_stats)
    {
        if(NULL != st->filename)
            xh_map_hook(xh_core_maps, st->filename, st->symbol, st->new_func, st->old_func);
    }

    xh_map_hook_finish(xh_core_maps);
    return r;
}

#if 0

//unavailable on Android 7.0+
//hook for dlopen()
static void *(*xh_core_dlopen_old)(const char *, int) = NULL;
static void *xh_core_dlopen_new(const char *filename, int flags)
{
    void *r = NULL;
    
    XH_LOG_INFO("catch dlopen(%s, %d)\n", filename, flags);

    if(NULL != xh_core_dlopen_old)
    {
        r = xh_core_dlopen_old(filename, flags);

        pthread_mutex_lock(&xh_core_mutex);
        xh_core_refresh_impl();
        pthread_mutex_unlock(&xh_core_mutex);
    }
    return r;
}

//unavailable on Android 7.0+
//hook for android_dlopen_ext()
static void *(*xh_core_android_dlopen_ext_old)(const char *, int, const void *) = NULL;
static void *xh_core_android_dlopen_ext_new(const char *filename, int flags, const void *extinfo)
{
    void *r = NULL;
    
    XH_LOG_INFO("catch android_dlopen_ext(%s, %d, %p)\n", filename, flags, extinfo);

    if(NULL != xh_core_android_dlopen_ext_old)
    {
        r = xh_core_android_dlopen_ext_old(filename, flags, extinfo);

        pthread_mutex_lock(&xh_core_mutex);
        xh_core_refresh_impl();
        pthread_mutex_unlock(&xh_core_mutex);
    }
    return r;
}

//unavailable on Android 7.0+
//hook for dlclose()
static int (*xh_core_dlclose_old)(void *) = NULL;
static int xh_core_dlclose_new(void *handle)
{
    int r = 0;
    
    XH_LOG_INFO("catch dlclose(%p)\n", handle);

    if(NULL != xh_core_dlclose_old)
    {
        r = xh_core_dlclose_old(handle);

        pthread_mutex_lock(&xh_core_mutex);
        xh_core_refresh_impl();
        pthread_mutex_unlock(&xh_core_mutex);
    }
    return r;
}

#endif

int xh_core_refresh()
{
    int r = 0;
    
    pthread_mutex_lock(&xh_core_mutex);

    if(NULL == xh_core_maps)
    {
        //do init here
        if(0 != (r = xh_map_create(&xh_core_maps)))
        {
            xh_core_maps = NULL;
            goto end;
        }
        XH_LOG_INFO("%s\n", xh_version_str_full());
        
        //unavailable on Android 7.0+
        //xh_core_hook_impl(NULL, "dlopen", xh_core_dlopen_new, (void **)(&xh_core_dlopen_old));
        //xh_core_hook_impl(NULL, "android_dlopen_ext", xh_core_android_dlopen_ext_new, (void **)(&xh_core_android_dlopen_ext_old));
        //xh_core_hook_impl(NULL, "dlclose", xh_core_dlclose_new, (void **)(&xh_core_dlclose_old));
        
#if XH_CORE_DEBUG
        xh_core_dump();
#endif
    }

    r = xh_core_refresh_impl();

 end:
    pthread_mutex_unlock(&xh_core_mutex);
    return r;
}

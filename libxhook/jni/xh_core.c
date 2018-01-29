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

//filename -> new_func, old_func
typedef struct xh_core_stat_filename
{
    const char  *filename;
    void        *new_func;
    void       **old_func;
    RB_ENTRY(xh_core_stat_filename) link;    
} xh_core_stat_filename_t;
static __inline__ int xh_core_stat_filename_cmp(xh_core_stat_filename_t *a, xh_core_stat_filename_t *b)
{
    if(NULL == a->filename && NULL == b->filename)
        return 0;
    else if(NULL != a->filename && NULL == b->filename)
        return 1;
    else if(NULL == a->filename && NULL != b->filename)
        return -1;
    else
        return strcmp(a->filename, b->filename);
}
typedef RB_HEAD(xh_core_stat_filename_tree, xh_core_stat_filename) xh_core_stat_filename_tree_t;
RB_GENERATE_STATIC(xh_core_stat_filename_tree, xh_core_stat_filename, link, xh_core_stat_filename_cmp)

//symbol -> filename(s)
typedef struct xh_core_stat_symbol
{
    const char                   *symbol;
    xh_core_stat_filename_tree_t  filename_tree;
    int                           have_wildcard;
    RB_ENTRY(xh_core_stat_symbol) link;
} xh_core_stat_symbol_t;
static __inline__ int xh_core_stat_symbol_cmp(xh_core_stat_symbol_t *a, xh_core_stat_symbol_t *b)
{
    return strcmp(a->symbol, b->symbol);
}
typedef RB_HEAD(xh_core_stat_symbol_tree, xh_core_stat_symbol) xh_core_stat_symbol_tree_t;
RB_GENERATE_STATIC(xh_core_stat_symbol_tree, xh_core_stat_symbol, link, xh_core_stat_symbol_cmp)

static pthread_mutex_t             xh_core_mutex        = PTHREAD_MUTEX_INITIALIZER;
static xh_core_stat_symbol_tree_t  xh_core_stat_symbols = RB_INITIALIZER(NULL);
static xh_map_t                   *xh_core_maps         = NULL;

void xh_core_set_log_priority(android_LogPriority priority)
{
    xh_log_priority = priority;
}

static int xh_core_hook_impl(const char *filename, const char *symbol, void *new_func, void **old_func)
{
    xh_core_stat_symbol_t    st_symbol_key   = {.symbol = symbol};
    xh_core_stat_symbol_t   *st_symbol       = NULL;
    xh_core_stat_filename_t  st_filename_key = {.filename = filename};
    xh_core_stat_filename_t *st_filename     = NULL;

    if(NULL == symbol || 0 == new_func) return XH_ERRNO_INVAL;

    if(NULL != (st_symbol = RB_FIND(xh_core_stat_symbol_tree, &xh_core_stat_symbols, &st_symbol_key)) &&
       NULL != RB_FIND(xh_core_stat_filename_tree, &(st_symbol->filename_tree), &st_filename_key))
        return XH_ERRNO_REPEAT;

    if(NULL == st_symbol)
    {
        //new symbol info
        if(NULL == (st_symbol = malloc(sizeof(xh_core_stat_symbol_t)))) return XH_ERRNO_NOMEM;
        if(NULL == (st_symbol->symbol = strdup(symbol)))
        {
            free(st_symbol);
            return XH_ERRNO_NOMEM;
        }
        RB_INIT(&(st_symbol->filename_tree));
        st_symbol->have_wildcard = 0;

        //insert symbol info to tree
        RB_INSERT(xh_core_stat_symbol_tree, &xh_core_stat_symbols, st_symbol);
    }

    //new filename info
    if(NULL == (st_filename = malloc(sizeof(xh_core_stat_filename_t)))) return XH_ERRNO_NOMEM;
    if(NULL != filename)
    {
        if(NULL == (st_filename->filename = strdup(filename)))
        {
            free(st_filename);
            return XH_ERRNO_NOMEM;
        }
    }
    else
    {
        st_filename->filename = NULL;
        st_symbol->have_wildcard = 1;
    }
    st_filename->new_func = new_func;
    st_filename->old_func = old_func;

    //insert filename info into symbol info
    RB_INSERT(xh_core_stat_filename_tree, &(st_symbol->filename_tree), st_filename);
    
    return 0;
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
    xh_core_stat_symbol_t   *st_symbol;
    xh_core_stat_filename_t *st_filename;

    XH_LOG_DEBUG("Stat:\n");
    RB_FOREACH(st_symbol, xh_core_stat_symbol_tree, &xh_core_stat_symbols)
    {
        RB_FOREACH(st_filename, xh_core_stat_filename_tree, &(st_symbol->filename_tree))
        {
            XH_LOG_INFO("  %s : %s : %p -> %p\n",
                        st_symbol->symbol,
                        (NULL == st_filename->filename ? "*" : st_filename->filename),
                        (st_filename->old_func ? *(st_filename->old_func) : 0),
                        st_filename->new_func);
        }
    }
}
#endif

static int xh_core_need_hook_check(const char *pathname, const char *filename, void *arg)
{
    xh_core_stat_symbol_t   *st_symbol;
    xh_core_stat_filename_t *st_filename;

    if(NULL != filename)
    {
        if(NULL != strstr(pathname, filename))
            return 1;
        else
            return 0;
    }
    else
    {
        st_symbol = (xh_core_stat_symbol_t *)arg;
        RB_FOREACH(st_filename, xh_core_stat_filename_tree, &(st_symbol->filename_tree))
        {
            if(NULL != st_filename->filename && NULL != strstr(pathname, st_filename->filename))
                return 0;
        }
        return 1;
    }
}

static int xh_core_refresh_impl()
{
    xh_core_stat_symbol_t   *st_symbol;
    xh_core_stat_filename_t *st_filename;
    int                      r;

    if(0 != (r = xh_map_refresh(xh_core_maps))) return r;

    RB_FOREACH(st_symbol, xh_core_stat_symbol_tree, &xh_core_stat_symbols)
    {
        RB_FOREACH(st_filename, xh_core_stat_filename_tree, &(st_symbol->filename_tree))
        {
            xh_map_hook(xh_core_maps, st_filename->filename, st_symbol->symbol,
                        st_filename->new_func, st_filename->old_func,
                        xh_core_need_hook_check, (void *)st_symbol);
        }
    }
    
    xh_map_hook_finish(xh_core_maps);
    return 0;
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

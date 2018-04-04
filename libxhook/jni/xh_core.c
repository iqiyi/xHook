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
#include "xh_elf.h"
#include "xh_version.h"

#define XH_CORE_DEBUG 0

//filename -> new_func, old_func
typedef struct xh_core_filename
{
    const char  *filename;
    void        *new_func;
    void       **old_func;
    RB_ENTRY(xh_core_filename) link;
} xh_core_filename_t;
static __inline__ int xh_core_filename_cmp(xh_core_filename_t *a, xh_core_filename_t *b)
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
typedef RB_HEAD(xh_core_filename_tree, xh_core_filename) xh_core_filename_tree_t;
RB_GENERATE_STATIC(xh_core_filename_tree, xh_core_filename, link, xh_core_filename_cmp)

//symbol -> filename(s)
typedef struct xh_core_symbol
{
    const char              *symbol;
    xh_core_filename_tree_t  filenames;
    int                      have_wildcard;
    RB_ENTRY(xh_core_symbol) link;
} xh_core_symbol_t;
static __inline__ int xh_core_symbol_cmp(xh_core_symbol_t *a, xh_core_symbol_t *b)
{
    return strcmp(a->symbol, b->symbol);
}
typedef RB_HEAD(xh_core_symbol_tree, xh_core_symbol) xh_core_symbol_tree_t;
RB_GENERATE_STATIC(xh_core_symbol_tree, xh_core_symbol, link, xh_core_symbol_cmp)

static xh_core_symbol_tree_t  xh_core_symbols      = RB_INITIALIZER(NULL);
static xh_map_t              *xh_core_maps         = NULL;
static pthread_mutex_t        xh_core_mutex        = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t         xh_core_cond         = PTHREAD_COND_INITIALIZER;
static pthread_t              xh_core_refresh_tid;
static volatile int           xh_core_running      = 0;
static volatile int           xh_core_refresh_need = 0;
static int                    xh_core_system_hook  = 0;
static int                    xh_core_reldyn_hook  = 0;

void xh_core_set_log_priority(android_LogPriority priority)
{
    xh_log_priority = priority;
}

void xh_core_set_system_hook(int flag)
{
    xh_core_system_hook = flag;
}

void xh_core_set_reldyn_hook(int flag)
{
    xh_core_reldyn_hook = flag;
}

static int xh_core_hook_impl(const char *filename, const char *symbol, void *new_func, void **old_func)
{
    xh_core_symbol_t    sym_key  = {.symbol = symbol};
    xh_core_symbol_t   *sym      = NULL;
    xh_core_filename_t  file_key = {.filename = filename};
    xh_core_filename_t *file     = NULL;

    //find or create symbol info node
    sym = RB_FIND(xh_core_symbol_tree, &xh_core_symbols, &sym_key);
    if(NULL == sym)
    {
        //new symbol info
        if(NULL == (sym = malloc(sizeof(xh_core_symbol_t)))) return XH_ERRNO_NOMEM;
        if(NULL == (sym->symbol = strdup(symbol)))
        {
            free(sym);
            return XH_ERRNO_NOMEM;
        }
        RB_INIT(&(sym->filenames));

        //insert symbol info into tree
        RB_INSERT(xh_core_symbol_tree, &xh_core_symbols, sym);
    }

    //find or create filename info node
    file = RB_FIND(xh_core_filename_tree, &(sym->filenames), &file_key);
    if(NULL == file)
    {
        //new filename info
        if(NULL == (file = malloc(sizeof(xh_core_filename_t)))) return XH_ERRNO_NOMEM;
        if(NULL != filename)
        {
            if(NULL == (file->filename = strdup(filename)))
            {
                free(file);
                return XH_ERRNO_NOMEM;
            }
        }
        else
        {
            file->filename = NULL;
        }
        
        //insert filename info into symbol info
        RB_INSERT(xh_core_filename_tree, &(sym->filenames), file);
    }

    //update the filename info
    file->new_func = new_func;
    file->old_func = old_func;
    
    return 0;
}

int xh_core_hook(const char *filename, const char *symbol, void *new_func, void **old_func)
{
    int r;
    
    if(NULL == symbol || NULL == new_func) return XH_ERRNO_INVAL;
    
    if(xh_core_running) return XH_ERRNO_RUNNING;
    
    pthread_mutex_lock(&xh_core_mutex);
    r = xh_core_hook_impl(filename, symbol, new_func, old_func);
    pthread_mutex_unlock(&xh_core_mutex);
    return r;
}

int xh_core_unhook(const char *filename, const char *symbol)
{
    int r;
    
    if(NULL == filename || NULL == symbol) return XH_ERRNO_INVAL;
    
    if(xh_core_running) return XH_ERRNO_RUNNING;

    pthread_mutex_lock(&xh_core_mutex);
    r = xh_core_hook_impl(filename, symbol, NULL, NULL);
    pthread_mutex_unlock(&xh_core_mutex);
    return r;
}

#if XH_CORE_DEBUG
static void xh_core_dump()
{
    xh_core_symbol_t   *sym;
    xh_core_filename_t *file;

    XH_LOG_DEBUG("Stat:\n");
    RB_FOREACH(sym, xh_core_symbol_tree, &xh_core_symbols)
    {
        RB_FOREACH(file, xh_core_filename_tree, &(sym->filenames))
        {
            XH_LOG_INFO("  %s : %s : %p -> %p\n",
                        sym->symbol,
                        (NULL == file->filename ? "*" : file->filename),
                        (file->old_func ? *(file->old_func) : 0),
                        file->new_func);
        }
    }
}
#endif

static int xh_core_need_hook_check(const char *pathname, const char *filename, void *arg)
{
    xh_core_symbol_t   *sym = (xh_core_symbol_t *)arg;;
    xh_core_filename_t *file;

    if(NULL != filename) //not wildcard
    {
        //check for filename match
        if(NULL == strstr(pathname, filename)) return 0;

        //check for unhook info (new_func == NULL)
        RB_FOREACH(file, xh_core_filename_tree, &(sym->filenames))
        {
            if(NULL == file->new_func &&
               NULL != file->filename &&
               NULL != strstr(pathname, file->filename))
                return 0;
        }
        
        return 1;
    }
    else //wildcard
    {
        //check for special filename's hook or unhook info
        RB_FOREACH(file, xh_core_filename_tree, &(sym->filenames))
        {
            if(NULL != file->filename &&
               NULL != strstr(pathname, file->filename))
                return 0;
        }
        
        return 1;
    }
}

static void xh_core_refresh_impl()
{
    xh_core_symbol_t   *sym;
    xh_core_filename_t *file;

    if(0 != xh_map_refresh(xh_core_maps)) return;

    RB_FOREACH(sym, xh_core_symbol_tree, &xh_core_symbols)
    {
        RB_FOREACH(file, xh_core_filename_tree, &(sym->filenames))
        {
            if(NULL != file->new_func)
            {
                xh_map_hook(xh_core_maps, file->filename, sym->symbol,
                            file->new_func, file->old_func,
                            xh_core_need_hook_check, (void *)sym);
            }
        }
    }
    
    xh_map_hook_finish(xh_core_maps);
}

static void *xh_core_refresh_loop_func(void *arg)
{
    (void)arg;
    
    pthread_setname_np(xh_core_refresh_tid, "xh_refresh_loop");

    while(xh_core_running)
    {
        //waiting for a refresh task or exit
        pthread_mutex_lock(&xh_core_mutex);
        while(!xh_core_refresh_need && xh_core_running)
        {
            pthread_cond_wait(&xh_core_cond, &xh_core_mutex);
        }
        if(!xh_core_running)
        {
            pthread_mutex_unlock(&xh_core_mutex);
            break;
        }
        xh_core_refresh_need = 0;
        pthread_mutex_unlock(&xh_core_mutex);

        //do refresh
        xh_core_refresh_impl();
    }

    return NULL;
}

int xh_core_start()
{
    int r = 0;
    
    if(xh_core_running) return 0;

    pthread_mutex_lock(&xh_core_mutex);
    
    xh_elf_init_sig_handler();
    
    if(xh_core_running) goto end;

    //unavailable on Android 7.0+
    //register dlopen() and android_dlopen_ext() hooks for auto-refresh
    //xh_core_hook_dl();

    //create map
    if(0 != (r = xh_map_create(&xh_core_maps, xh_core_system_hook, xh_core_reldyn_hook))) goto end;

    //start refresh loop func
    xh_core_running = 1;
    if(0 != (r = pthread_create(&xh_core_refresh_tid, NULL, &xh_core_refresh_loop_func, NULL))) goto end;

    //do the first refresh
    xh_core_refresh_need = 1;
    pthread_cond_signal(&xh_core_cond);

    //OK
    XH_LOG_INFO("%s\n", xh_version_str_full());
    r = 0;
    
#if XH_CORE_DEBUG
    xh_core_dump();
#endif
    
 end:
    
    //something failed
    if(0 != r)
    {
        xh_core_running = 0;
        if(NULL != xh_core_maps)
            xh_map_destroy(&xh_core_maps);
    }
    
    pthread_mutex_unlock(&xh_core_mutex);
    return r;
}

int xh_core_stop()
{
    if(!xh_core_running) return 0;
    
    pthread_mutex_lock(&xh_core_mutex);
    xh_core_running = 0;
    xh_core_refresh_need = 0;
    pthread_cond_signal(&xh_core_cond);
    pthread_mutex_unlock(&xh_core_mutex);

    pthread_join(xh_core_refresh_tid, NULL);
    xh_map_destroy(&xh_core_maps);

    xh_elf_uninit_sig_handler();

    return 0;
}

int xh_core_refresh()
{
    if(!xh_core_running) return XH_ERRNO_NOTRUN;
    
    pthread_mutex_lock(&xh_core_mutex);
    xh_core_refresh_need = 1;
    pthread_cond_signal(&xh_core_cond);
    pthread_mutex_unlock(&xh_core_mutex);
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
        xh_core_refresh();
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
        xh_core_refresh();
    }
    return r;
}

//unavailable on Android 7.0+
static void xh_core_hook_dl()
{
    xh_core_hook_register_impl(NULL, "dlopen", xh_core_dlopen_new, (void **)(&xh_core_dlopen_old));
    xh_core_hook_register_impl(NULL, "android_dlopen_ext", xh_core_android_dlopen_ext_new, (void **)(&xh_core_android_dlopen_ext_old));
}

#endif

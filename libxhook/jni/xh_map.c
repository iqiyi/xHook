#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "xh_tree.h"
#include "xh_errno.h"
#include "xh_log.h"
#include "xh_elf.h"
#include "xh_map.h"

#define XH_MAP_FLAG_INITED    ((uint8_t)(1 << 0))
#define XH_MAP_FLAG_FAILED    ((uint8_t)(1 << 1))
#define XH_MAP_FLAG_REFRESHED ((uint8_t)(1 << 2))
#define XH_MAP_FLAG_HOOKED    ((uint8_t)(1 << 3))

#define XH_MAP_FLAG_CHECK(v, f) (((v) & (f)) == 0 ? 0 : 1)
#define XH_MAP_FLAG_ADD(v, f)   ((v) |= (f))
#define XH_MAP_FLAG_DEL(v, f)   ((v) &= ~(f))
#define XH_MAP_FLAG_CLEAN(v)    ((v) = 0)

typedef struct xh_map_item
{
    char      *pathname;
    uintptr_t  base_addr;
    xh_elf_t   elf;
    uint8_t    flag;
    RB_ENTRY(xh_map_item) link;
} xh_map_item_t;
static __inline__ int xh_map_item_cmp(xh_map_item_t *a, xh_map_item_t *b)
{
    return strcmp(a->pathname, b->pathname);
}
typedef RB_HEAD(xh_map_tree, xh_map_item) xh_map_tree_t;
RB_GENERATE_STATIC(xh_map_tree, xh_map_item, link, xh_map_item_cmp)

struct xh_map
{
    xh_map_tree_t maps;
};

int xh_map_create(xh_map_t **self)
{
    if(NULL == (*self = malloc(sizeof(xh_map_t)))) return XH_ERRNO_NOMEM;
    RB_INIT(&((*self)->maps));
    return 0;
}

int xh_map_destroy(xh_map_t **self)
{
    xh_map_item_t *mi = NULL, *mi_tmp = NULL;

    RB_FOREACH_SAFE(mi, xh_map_tree, &((*self)->maps), mi_tmp)
    {
        RB_REMOVE(xh_map_tree, &((*self)->maps), mi);
        if(mi->pathname) free(mi->pathname);
        free(mi);
    }
    free(*self);
    *self = NULL;
    return 0;
}

#if 0
static void xh_map_dump(xh_map_t *self)
{
    xh_map_item_t *mi = NULL;
    
    XH_LOG_DEBUG("Maps:\n");
    RB_FOREACH(mi, xh_map_tree, &(self->maps))
    {
        XH_LOG_DEBUG("  %"PRIxPTR" %s\n", mi->base_addr, mi->pathname);
    }
}
#endif

int xh_map_refresh(xh_map_t *self)
{
    char           line[1024];
    FILE          *fp;
    uintptr_t      base_addr;
    char           perm[5];
    int            pathname_pos;
    char          *pathname;
    size_t         pathname_len;
    xh_map_item_t *mi, *mi_tmp;
    xh_map_item_t  mi_key;

    if(NULL == (fp = fopen("/proc/self/maps", "r")))
    {
        XH_LOG_ERROR("map refresh failed, fopen /proc/self/maps failed");
        return XH_ERRNO_BADMAPS;
    }

    //set all refreshed-flag to false
    RB_FOREACH(mi, xh_map_tree, &(self->maps))
        XH_MAP_FLAG_DEL(mi->flag, XH_MAP_FLAG_REFRESHED);

    //start to refresh maps
    while(fgets(line, sizeof(line), fp))
    {
        if(sscanf(line, "%"PRIxPTR"-%*lx %4s %*x %*x:%*x %*d%n", &base_addr, perm, &pathname_pos) != 2) continue;

        //check permission
        if(perm[0] != 'r' || perm[2] != 'x' || perm[3] != 'p') continue;

        //get and check pathname
        while(isspace(line[pathname_pos]) && pathname_pos < (int)(sizeof(line) - 1))
            pathname_pos += 1;
        if(pathname_pos >= (int)(sizeof(line) - 1)) continue;
        pathname = line + pathname_pos;
        pathname_len = strlen(pathname);
        if(0 == pathname_len) continue;
        if(pathname[pathname_len - 1] == '\n')
        {
            pathname[pathname_len - 1] = '\0';
            pathname_len -= 1;
        }
        if(0 == pathname_len) continue;        
        if('[' == pathname[0]) continue;

        //check elf
        if(0 != xh_elf_check_elfheader(base_addr)) continue;
        
        //check existed pathname and base_addr, update the refreshed-flag
        mi_key.pathname = pathname;
        if(NULL != (mi = RB_FIND(xh_map_tree, &(self->maps), &mi_key)))
        {
            if(mi->base_addr != base_addr)
            {
                mi->base_addr = base_addr; //base_addr changed
                xh_elf_reset(&(mi->elf));
                XH_MAP_FLAG_CLEAN(mi->flag); //need to be init and hook again
            }
            XH_MAP_FLAG_ADD(mi->flag, XH_MAP_FLAG_REFRESHED);
            continue;
        }

        //add new item
        if(NULL == (mi = (xh_map_item_t *)malloc(sizeof(xh_map_item_t)))) continue;
        if(NULL == (mi->pathname = strdup(pathname)))
        {
            free(mi);
            continue;
        }
        mi->base_addr = base_addr;
        xh_elf_reset(&(mi->elf));
        XH_MAP_FLAG_CLEAN(mi->flag);
        XH_MAP_FLAG_ADD(mi->flag, XH_MAP_FLAG_REFRESHED);
        if(NULL != RB_INSERT(xh_map_tree, &(self->maps), mi))
            free(mi);
    }
    fclose(fp);

    //free all unfreshed item, maybe dlclosed?
    RB_FOREACH_SAFE(mi, xh_map_tree, &(self->maps), mi_tmp)
    {
        if(!XH_MAP_FLAG_CHECK(mi->flag, XH_MAP_FLAG_REFRESHED))
        {
            RB_REMOVE(xh_map_tree, &(self->maps), mi);
            if(mi->pathname) free(mi->pathname);
            free(mi);
        }
    }

    XH_LOG_INFO("map refreshed");
    return 0;
}

int xh_map_hook(xh_map_t *self, const char *filename, const char *symbol,
                void *new_func, void **old_func,
                xh_map_need_hook_check_t need_hook_check, void *arg)
{
    xh_map_item_t *mi  = NULL;
    int            r   = 0;
    int            ret = 0;
    
    RB_FOREACH(mi, xh_map_tree, &(self->maps))
    {
        //save the first error's number
        if(0 != r && 0 == ret) ret = r;
            
        if(XH_MAP_FLAG_CHECK(mi->flag, XH_MAP_FLAG_FAILED)) continue;
        if(XH_MAP_FLAG_CHECK(mi->flag, XH_MAP_FLAG_HOOKED)) continue;

        if(need_hook_check(mi->pathname, filename, arg))
        {
            if(!XH_MAP_FLAG_CHECK(mi->flag, XH_MAP_FLAG_INITED))
            {
                XH_MAP_FLAG_ADD(mi->flag, XH_MAP_FLAG_INITED);
                //init
                if(0 != (r = xh_elf_init(&(mi->elf), mi->base_addr, mi->pathname)))
                {
                    XH_MAP_FLAG_ADD(mi->flag, XH_MAP_FLAG_FAILED);
                    continue;
                }
            }

            //hook
            if(0 != (r = xh_elf_hook(&(mi->elf), symbol, new_func, old_func)))
            {
                XH_MAP_FLAG_ADD(mi->flag, XH_MAP_FLAG_FAILED);
                continue;
            }
        }
    }

    return ret;
}

void xh_map_hook_finish(xh_map_t *self)
{
    xh_map_item_t *mi = NULL;
    
    RB_FOREACH(mi, xh_map_tree, &(self->maps))
        if(!XH_MAP_FLAG_CHECK(mi->flag, XH_MAP_FLAG_HOOKED))
            XH_MAP_FLAG_ADD(mi->flag, XH_MAP_FLAG_HOOKED);
}

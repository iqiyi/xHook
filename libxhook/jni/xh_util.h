#ifndef XH_UTILS_H
#define XH_UTILS_H 1

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__LP64__)
#define XH_UTIL_FMT_LEN     "16"
#define XH_UTIL_FMT_X       "llx"
#else
#define XH_UTIL_FMT_LEN     "8"
#define XH_UTIL_FMT_X       "x"
#endif

#define XH_UTIL_FMT_FIXED_X XH_UTIL_FMT_LEN XH_UTIL_FMT_X
#define XH_UTIL_FMT_FIXED_S XH_UTIL_FMT_LEN "s"

int xh_util_get_mem_protect(uintptr_t addr, size_t len, const char *pathname, unsigned int *prot);
int xh_util_get_addr_protect(uintptr_t addr, const char *pathname, unsigned int *prot);
int xh_util_set_addr_protect(uintptr_t addr, unsigned int prot);
void xh_util_flush_instruction_cache(uintptr_t addr);

#ifdef __cplusplus
}
#endif

#endif

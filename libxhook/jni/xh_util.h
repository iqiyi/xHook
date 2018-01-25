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


#ifdef __cplusplus
}
#endif

#endif

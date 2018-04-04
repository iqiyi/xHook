#include "xh_version.h"

#define XH_VERSION_MAJOR 1
#define XH_VERSION_MINOR 0
#define XH_VERSION_EXTRA 12

#define XH_VERSION ((XH_VERSION_MAJOR << 16) | (XH_VERSION_MINOR <<  8) | (XH_VERSION_EXTRA))

#define XH_VERSION_TO_STR_HELPER(x) #x
#define XH_VERSION_TO_STR(x) XH_VERSION_TO_STR_HELPER(x)

#define XH_VERSION_STR XH_VERSION_TO_STR(XH_VERSION_MAJOR) "." \
                       XH_VERSION_TO_STR(XH_VERSION_MINOR) "." \
                       XH_VERSION_TO_STR(XH_VERSION_EXTRA)

#if defined(__arm__)
#define XH_VERSION_ARCH "arm"
#elif defined(__aarch64__)
#define XH_VERSION_ARCH "aarch64"
#else
#define XH_VERSION_ARCH "unknown"
#endif

#define XH_VERSION_STR_FULL "libxhook "XH_VERSION_STR" ("XH_VERSION_ARCH")"

unsigned int xh_version()
{
    return XH_VERSION;
}

const char *xh_version_str()
{
    return XH_VERSION_STR;
}

const char *xh_version_str_full()
{
    return XH_VERSION_STR_FULL;
}

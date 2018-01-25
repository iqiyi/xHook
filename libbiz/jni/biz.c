#include <stdio.h>
#include <jni.h>
#include <android/log.h>
#include "xh_core.h"

static int my_common_log_print(int prio, const char* tag, const char* fmt, ...)
{
    va_list ap;
    char buf[1024];
    int r;
    
    snprintf(buf, sizeof(buf), "[%s] %s", (NULL == tag ? "" : tag), (NULL == fmt ? "" : fmt));

    va_start(ap, fmt);
    r = __android_log_vprint(prio, "xhook_common", buf, ap);
    va_end(ap);
    return r;
}

static int my_libtest_log_print(int prio, const char* tag, const char* fmt, ...)
{
    va_list ap;
    char buf[1024];
    int r;
    
    snprintf(buf, sizeof(buf), "[%s] %s", (NULL == tag ? "" : tag), (NULL == fmt ? "" : fmt));

    va_start(ap, fmt);
    r = __android_log_vprint(prio, "xhook_libtest", buf, ap);
    va_end(ap);
    return r;
}

void Java_com_qiyi_biz_NativeHandler_hook(JNIEnv* env, jobject obj)
{
    (void)env;
    (void)obj;
    
    xh_core_hook(NULL, "__android_log_print", my_common_log_print, NULL);
    xh_core_hook("libtest.so", "__android_log_print", my_libtest_log_print, NULL);
}

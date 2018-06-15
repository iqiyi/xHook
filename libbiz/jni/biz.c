#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <jni.h>
#include <android/log.h>
#include "xhook.h"

static int my_system_log_print(int prio, const char* tag, const char* fmt, ...)
{
    va_list ap;
    char buf[1024];
    int r;
    
    snprintf(buf, sizeof(buf), "[%s] %s", (NULL == tag ? "" : tag), (NULL == fmt ? "" : fmt));

    va_start(ap, fmt);
    r = __android_log_vprint(prio, "xhook_system", buf, ap);
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

void Java_com_qiyi_biz_NativeHandler_start(JNIEnv* env, jobject obj)
{
    (void)env;
    (void)obj;

    xhook_register("^/system/.*\\.so$",  "__android_log_print", my_system_log_print,  NULL);
    xhook_register("^/vendor/.*\\.so$",  "__android_log_print", my_system_log_print,  NULL);
    xhook_register(".*/libtest\\.so$", "__android_log_print", my_libtest_log_print, NULL);

    //just for testing
    xhook_ignore(".*/liblog\\.so$", "__android_log_print"); //ignore __android_log_print in liblog.so
    xhook_ignore(".*/libjavacore\\.so$", NULL); //ignore all hooks in libjavacore.so
}

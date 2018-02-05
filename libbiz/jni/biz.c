#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
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

static void *new_thread_func(void *arg)
{
    (void)arg;    
    unsigned int i = 0;
    
    while(1)
    {
        __android_log_print(ANDROID_LOG_DEBUG, "mybiz", "call directly. %u\n", i);
        i++;
        sleep(1);
    }
    
    return NULL;
}

void Java_com_qiyi_biz_NativeHandler_start(JNIEnv* env, jobject obj)
{
    (void)env;
    (void)obj;

    pthread_t tid;
    pthread_create(&tid, NULL, &new_thread_func, NULL);
    
    xh_core_hook(NULL, "__android_log_print", my_common_log_print, NULL);
    xh_core_hook("libtest.so", "__android_log_print", my_libtest_log_print, NULL);
    xh_core_unhook("libbiz.so", "__android_log_print");
}

#include <jni.h>
#include "xh_core.h"

#define JNI_API_DEF(f) Java_com_qiyi_xhook_NativeHandler_##f

JNIEXPORT void JNI_API_DEF(enableDebug)(JNIEnv *env, jobject obj, jboolean flag)
{
    (void)env;
    (void)obj;

    xh_core_set_log_priority(flag ? ANDROID_LOG_INFO : ANDROID_LOG_WARN);
}

JNIEXPORT void JNI_API_DEF(enableSystemHook)(JNIEnv *env, jobject obj, jboolean flag)
{
    (void)env;
    (void)obj;

    xh_core_set_system_hook(flag ? 1 : 0);
}

JNIEXPORT void JNI_API_DEF(enableReldynHook)(JNIEnv *env, jobject obj, jboolean flag)
{
    (void)env;
    (void)obj;

    xh_core_set_reldyn_hook(flag ? 1 : 0);
}

JNIEXPORT jint JNI_API_DEF(start)(JNIEnv *env, jobject obj)
{
    (void)env;
    (void)obj;

    return xh_core_start();
}

JNIEXPORT jint JNI_API_DEF(stop)(JNIEnv *env, jobject obj)
{
    (void)env;
    (void)obj;

    return xh_core_stop();
}

JNIEXPORT jint JNI_API_DEF(refresh)(JNIEnv *env, jobject obj)
{
    (void)env;
    (void)obj;

    return xh_core_refresh();
}

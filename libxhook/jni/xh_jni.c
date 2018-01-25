#include <jni.h>
#include "xh_core.h"

#define JNI_API_DEF(f) Java_com_qiyi_xhook_NativeHandler_##f

JNIEXPORT void JNI_API_DEF(refresh)(JNIEnv *env, jobject obj)
{
    (void)env;
    (void)obj;

    xh_core_refresh();
}

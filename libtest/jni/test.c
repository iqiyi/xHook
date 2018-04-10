#include <unistd.h>
#include <pthread.h>
#include <jni.h>
#include <android/log.h>

typedef int (*my_log_t)(int prio, const char* tag, const char* fmt, ...);
my_log_t my_global_log_ptr = (my_log_t)__android_log_print;

static void *new_thread_func(void *arg)
{
    (void)arg;
    my_log_t my_local_log_ptr2 = (my_log_t)__android_log_print;
    unsigned int i = 0;
    
    while(1)
    {
        my_log_t my_local_log_ptr = (my_log_t)__android_log_print;
        __android_log_print(ANDROID_LOG_DEBUG, "mytest", "call directly. %u\n", i);
        my_global_log_ptr(ANDROID_LOG_DEBUG, "mytest", "call from global ptr. %u\n", i);
        my_local_log_ptr(ANDROID_LOG_DEBUG, "mytest", "call from local ptr. %u\n", i);
        my_local_log_ptr2(ANDROID_LOG_DEBUG, "mytest", "call from local ptr2. %u (definitely failed when compiled with -O0)\n", i);
        i++;
        sleep(1);
    }
    
    return NULL;
}

void Java_com_qiyi_test_NativeHandler_start(JNIEnv* env, jobject obj)
{
    (void)env;
    (void)obj;
    
    pthread_t tid;
    pthread_create(&tid, NULL, &new_thread_func, NULL);
}

```

              oooo                            oooo        
              `888                            `888        
  oooo    ooo  888 .oo.    .ooooo.   .ooooo.   888  oooo  
   `88b..8P'   888P"Y88b  d88' `88b d88' `88b  888 .8P'   
     Y888'     888   888  888   888 888   888  888888.    
   .o8"'88b    888   888  888   888 888   888  888 `88b.  
  o88'   888o o888o o888o `Y8bod8P' `Y8bod8P' o888o o888o 

```

xhook
=====

xhook is a PLT (Procedure Linkage Table) hook library for Android native ELF (executable and shared libraries).

xhook has been keeping optimized for stability and compatibility.


Features
--------

* Support Android 4.0+ (API level 14+).
* Support armeabi, armeabi-v7a and arm64-v8a.
* Support to lookup symbol via **ELF HASH** and **GNU HASH**.
* Support to lookup reloc via **ELF reloc** and **ANDROID reloc**.
* Do **NOT** need root permission.
* Do not depends on any third-party shared libraries.
* Pure C code. Small library size.


Build
-----

You need google NDK for building xhook.
See: https://developer.android.com/ndk/downloads/index.html

The latest version of xhook is developed and debugged with the NDK version **r16b**.

* build the libraries (libxhook.so and other libraries for test)

```
./build_libs.sh
```

* install the libraries to test project's libs path

```
./install_libs.sh
```

* clean the libraries

```
./clean_libs.sh
```


Demo
----

```
cd ./xhookwrapper/
./gradlew assembleDebug
adb install ./app/build/outputs/apk/debug/app-debug.apk
```


API
----

External API header file: `libxhook/jni/xhook.h`

* **Register hook info**

```
int xhook_register(const char  *pathname_regex_str,  
                   const char  *symbol,  
                   void        *new_func,  
                   void       **old_func);
```

In current process's memory space, in every loaded ELF which pathname matches regular expression `pathname_regex_str`, every PLT entries to `symbol` will be **replaced with** `new_func`. The original one will be saved in `old_func`.

The `new_func` **must** have the same function declaration as the original one.

Return zero if successful, non-zero otherwise.

The regular expression for `pathname_regex_str` only support **POSIX BRE**.

* **Do hook**

```
int xhook_refresh(int async);
```

Do the real hook operations according to the registered hook info.

Pass `1` to `async` for asynchronous hook. Pass `0` to `async` for synchronous hook.

Return zero if successful, non-zero otherwise.

xhook will keep a global cache for saving the last ELF loading info from `/proc/self/maps`. This cache will also be updated in `xhook_refresh`. With this cache, `xhook_refresh` can determine which ELF is newly loaded. We only need to do hook in these newly loaded ELF.

* **Clear cache**

```
void xhook_clear();
```

Clear all cache owned by xhook, reset all global flags to default value.

If you confirm that all PLT entries you want have been hooked, you could call this function to save some memory.

* **Enable/Disable debug info**

```
void xhook_enable_debug(int flag);
```

Pass `1` to `flag` for enable debug info. Pass `0` to `flag` for disable. (**disabled** by default)

Debug info will be sent to logcat with tag `xhook`.

* **Enable/Disable SFP (segmentation fault protection)**

```
void xhook_enable_sigsegv_protection(int flag);
```

Pass `1` to `flag` for enable SFP. Pass `0` to `flag` for disable. (**enabled** by default) 

**xhook is NOT a compliant business layer library. We have to calculate the value of some pointers directly. Reading or writing the memory pointed to by these pointers will cause a segmentation fault in some unusual situations and environment. The APP crash rate increased which caused by xhook is about one ten-millionth (0.0000001) according to our test. (The increased crash rate is also related to the ELFs and symbols you need to hook). Finally, we have to use some trick to ignore this harmless crashing. We called it SFP (segmentation fault protection) which consists of: `sigaction()`, `SIGSEGV`, `siglongjmp()` and `sigsetjmp()`.**

**You should always enable SFP for release-APP, this will prevent your app from crashing. On the other hand, you should always disable SFP for debug-APP, so you can't miss any common coding mistakes that should be fixed.**


Examples
--------

```
//detect memory leaks
xhook_register(".*\\.so$", "malloc",  my_malloc,  NULL);
xhook_register(".*\\.so$", "calloc",  my_calloc,  NULL);
xhook_register(".*\\.so$", "realloc", my_realloc, NULL);
xhook_register(".*\\.so$", "free",    my_free,    NULL);

//inspect sockets lifecycle
xhook_register(".*\\.so$", "getaddrinfo", my_getaddrinfo, NULL);
xhook_register(".*\\.so$", "socket",      my_socket,      NULL);
xhook_register(".*\\.so$", "setsockopt"   my_setsockopt,  NULL);
xhook_register(".*\\.so$", "bind",        my_bind,        NULL);
xhook_register(".*\\.so$", "listen",      my_listen,      NULL);
xhook_register(".*\\.so$", "connect",     my_connect,     NULL);
xhook_register(".*\\.so$", "shutdown",    my_shutdown,    NULL);
xhook_register(".*\\.so$", "close",       my_close,       NULL);

//filter off and save some android log to local file
xhook_register(".*\\.so$", "__android_log_write",  my_log_write,  NULL);
xhook_register(".*\\.so$", "__android_log_print",  my_log_print,  NULL);
xhook_register(".*\\.so$", "__android_log_vprint", my_log_vprint, NULL);
xhook_register(".*\\.so$", "__android_log_assert", my_log_assert, NULL);

//tracking
xhook_register("^/system/.*$", "mmap",   my_mmap,   NULL);
xhook_register("^/vendor/.*$", "munmap", my_munmap, NULL);

//defense to some injection attacks
xhook_register(".*com\\.hacker.*\\.so$", "malloc",  my_malloc_always_return_NULL, NULL);
xhook_register(".*/libhacker\\.so$",     "connect", my_connect_with_recorder,     NULL);

//fix some system bug
xhook_register(".*some_vendor.*/libvictim\\.so$", "bad_func", my_nice_func, NULL);

//hook now!
xhook_refresh(1);
```

License
-------

Copyright (c) 2018-present, iQIYI, Inc. All rights reserved.

Most source code in xhook are MIT licensed. Some other source code have BSD-style licenses.

Please refer to the LICENSE file for detailed information.


Contact
-------

github: https://github.com/iqiyi/xhook

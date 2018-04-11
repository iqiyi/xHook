xhook
======

xhook is a PLT (Procedure Linkage Table) hook library for Android native ELF (executable and shared library).

xhook has been keeping optimized on stability and compatibility.


Feature
-------

* Supports Android 4.0+ (API level 14+).
* Supports armeabi, armeabi-v7a and arm64-v8a.
* Supports to lookup symbol via **ELF HASH** and **GNU HASH**.
* Supports to lookup reloc via **ELF reloc** and **ANDROID reloc**.
* Do **NOT** need root permission.
* Do not depends on any third-party libraries.
* Pure C code. Small library size.


Build
-----

You need google NDK for building xhook.

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


APIs
----

* **Register hook info**

```
int xhook_register(const char  *pathname_regex_str,  
                   const char  *symbol,  
                   void        *new_func,  
                   void       **old_func);
```

In current process's memory space, in every loaded ELF which pathname matches regular expression `pathname_regex_str`, every call entries to `symbol` will be **replaced with** `new_func`. The original one will be saved in `old_func`.

The `new_func` **must** have the same function declaration as the original one.

The regular expression for `pathname_regex_str` only support **POSIX BRE**.

* **Do hook**

```
int xhook_refresh(int async);
```

Do hook according to the registered hook info.

Pass `1` to `async` for asynchronous hook. Pass `0` to `async` for synchronous hook.

xhook will keep a global cache for saving the last ELF loading info from `/proc/self/maps`. This cache will also be updated in `xhook_refresh`. With this cache, `xhook_refresh` can determine which ELF is newly loaded. We only need to do hook in these newly loaded ELF.


* **Clear cache**

```
void xhook_clear();
```

Clear all cache owned by xhook, reset all global flags to default value.

If you confirm that all call entries you want have been hooked, you could call this function to save some memory.

* **Enable/Disable debug info**

```
void xhook_enable_debug(int flag);
```

Pass `1` to `flag` for enable debug info. Pass `0` to `flag` for disable debug info. (xhook disable debug info by default)

Debug info will be sent to logcat with tag `xhook`.


Example
-------

```
//monitor memory leak
xhook_register(".*\\.so$", "malloc",  my_malloc,  NULL);
xhook_register(".*\\.so$", "calloc",  my_calloc,  NULL);
xhook_register(".*\\.so$", "realloc", my_realloc, NULL);
xhook_register(".*\\.so$", "free",    my_free,    NULL);

//monitor socket life cycle
xhook_register(".*\\.so$", "getaddrinfo", my_getaddrinfo, NULL);
xhook_register(".*\\.so$", "socket",      my_socket,      NULL);
xhook_register(".*\\.so$", "bind",        my_bind,        NULL);
xhook_register(".*\\.so$", "listen",      my_listen,      NULL);
xhook_register(".*\\.so$", "connect",     my_connect,     NULL);
xhook_register(".*\\.so$", "close",       my_close,       NULL);

//filter and save some android log to local file
xhook_register(".*\\.so$", "__android_log_write",  my_log_write,  NULL);
xhook_register(".*\\.so$", "__android_log_print",  my_log_print,  NULL);
xhook_register(".*\\.so$", "__android_log_vprint", my_log_vprint, NULL);
xhook_register(".*\\.so$", "__android_log_assert", my_log_assert, NULL);

//track some calls
xhook_register("^/system/.*$", "getaddrinfo", my_track_dns, NULL);
xhook_register("^/vendor/.*$", "getaddrinfo", my_track_dns, NULL);

//defense some injection attack
xhook_register(".*/libavb\\.so$",   "hack_connect", connect, NULL);
xhook_register(".*/libchost\\.so$", "hack_connect", connect, NULL);
xhook_register(".*com\\.qihoo.*/.*\\.so$", "malloc", my_malloc_always_return_NULL, NULL);

//fix some system bug
xhook_register(".*samsung.*/libEGL\\.so$", "bad_code", my_nice_code, NULL);

//hook now!
xhook_refresh(1);
```

License
-------

Copyright (c) 2018 iQiYi Inc. All rights reserved.

Use of this source code is governed by a MIT-style license that can be found in the LICENSE file.

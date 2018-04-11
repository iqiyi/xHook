//Use of this source code is governed by a MIT-style
//license that can be found in LICENSE file.
//
//Copyright (c) 2018 iQiYi Inc. All rights reserved.
//
#ifndef XHOOK_H
#define XHOOK_H 1

#ifdef __cplusplus
extern "C" {
#endif

#define XHOOK_EXPORT __attribute__((visibility("default")))

int xhook_register(const char *pathname_regex_str, const char *symbol,
                   void *new_func, void **old_func) XHOOK_EXPORT;

int xhook_refresh(int async) XHOOK_EXPORT;

void xhook_enable_debug(int flag) XHOOK_EXPORT;

void xhook_clear() XHOOK_EXPORT;

#ifdef __cplusplus
}
#endif

#endif

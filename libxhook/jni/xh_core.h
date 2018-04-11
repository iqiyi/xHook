//Use of this source code is governed by a MIT-style
//license that can be found in LICENSE file.
//
//Copyright (c) 2018 iQiYi Inc. All rights reserved.
//
#ifndef XH_CORE_H
#define XH_CORE_H 1

#ifdef __cplusplus
extern "C" {
#endif

int xh_core_register(const char *pathname_regex_str, const char *symbol,
                     void *new_func, void **old_func);

int xh_core_refresh(int async);

void xh_core_enable_debug(int flag);

void xh_core_clear();

#ifdef __cplusplus
}
#endif

#endif

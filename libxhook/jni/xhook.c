//Use of this source code is governed by a MIT-style
//license that can be found in LICENSE file.
//
//Copyright (c) 2018 iQiYi Inc. All rights reserved.
//
#include "xh_core.h"
#include "xhook.h"

int xhook_register(const char *pathname_regex_str, const char *symbol,
                   void *new_func, void **old_func)
{
    return xh_core_register(pathname_regex_str, symbol, new_func, old_func);
}

int xhook_refresh(int async)
{
    return xh_core_refresh(async);
}

void xhook_enable_debug(int flag)
{
    return xh_core_enable_debug(flag);
}

void xhook_clear()
{
    return xh_core_clear();
}

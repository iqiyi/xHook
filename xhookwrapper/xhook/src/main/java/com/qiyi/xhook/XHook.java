package com.qiyi.xhook;

import android.content.Context;
import android.util.Log;

/**
 * Created by caikelun on 18/01/2018.
 */

public class XHook {
    private static final XHook ourInstance = new XHook();
    private static boolean inited = false;

    public static XHook getInstance() {
        return ourInstance;
    }

    private XHook() {
    }

    /**
     * Check if xhook has inited.
     * @return true if xhook has inited, false otherwise.
     */
    public synchronized boolean isInited() {
        return inited;
    }

    /**
     * Init xhook.
     * @param ctx The application context.
     * @return true if successful, false otherwise.
     */
    public synchronized boolean init(Context ctx) {
        if(inited) {
            return true;
        }

        try {
            System.loadLibrary("xhook");
            inited = true;
        } catch (Throwable e) {
            try {
                System.load(ctx.getFilesDir().getParent() + "/lib/libxhook.so");
                inited = true;
            } catch (Throwable ex) {
                ex.printStackTrace();
                Log.e("xhook", "load libxhook.so failed");
            }
        }
        return inited;
    }

    /**
     * Re-hook after System.loadLibrary() and System.load().
     * @param async true if to refresh in async mode; otherwise, refresh in sync mode.
     * @return 0 if successful, false otherwise.
     */
    public synchronized void refresh(boolean async) {
        if(!inited) {
            return;
        }

        try {
            com.qiyi.xhook.NativeHandler.getInstance().refresh(async);
        } catch (Throwable ex) {
            ex.printStackTrace();
            Log.e("xhook", "xhook native refresh failed");
        }
    }

    /**
     * Clear all cache.
     */
    public synchronized void clear() {
        if(!inited) {
            return;
        }

        try {
            com.qiyi.xhook.NativeHandler.getInstance().clear();
        } catch (Throwable ex) {
            ex.printStackTrace();
            Log.e("xhook", "xhook native clear failed");
        }
    }

    /**
     * Enable/disable the debug log to logcat. (disabled by default)
     * @param flag the bool flag.
     */
    public synchronized void enableDebug(boolean flag) {
        if(!inited) {
            return;
        }

        try {
            com.qiyi.xhook.NativeHandler.getInstance().enableDebug(flag);
        } catch (Throwable ex) {
            ex.printStackTrace();
            Log.e("xhook", "xhook native enableDebug failed");
        }
    }

    /**
     * Enable/disable the segmentation fault protection. (enabled by default)
     * @param flag the bool flag.
     */
    public synchronized void enableSigSegvProtection(boolean flag) {
        if (!inited) {
            return;
        }

        try {
            com.qiyi.xhook.NativeHandler.getInstance().enableSigSegvProtection(flag);
        } catch (Throwable ex) {
            ex.printStackTrace();
            Log.e("xhook", "xhook native enableSigSegvProtection failed");
        }
    }
}

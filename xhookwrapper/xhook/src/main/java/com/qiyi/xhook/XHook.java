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
     * Enable/disable the debug log to logcat. (default is: disable)
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
     * Enable/disable hook System library. (default is: disable)
     * @param flag the bool flag.
     */
    public synchronized void enableSystemHook(boolean flag) {
        if(!inited) {
            return;
        }

        try {
            com.qiyi.xhook.NativeHandler.getInstance().enableSystemHook(flag);
        } catch (Throwable ex) {
            ex.printStackTrace();
            Log.e("xhook", "xhook native enableSystemHook failed");
        }
    }

    /**
     * Enable/disable hook .rel.dyn and .rel.android. (default is: disable)
     * @param flag the bool flag.
     */
    public synchronized void enableReldynHook(boolean flag) {
        if(!inited) {
            return;
        }

        try {
            com.qiyi.xhook.NativeHandler.getInstance().enableReldynHook(flag);
        } catch (Throwable ex) {
            ex.printStackTrace();
            Log.e("xhook", "xhook native enableReldynHook failed");
        }
    }

    /**
     * Start the hook mechanism.
     * @return 0 if successful, false otherwise.
     */
    public synchronized int start() {
        if(!inited) {
            return 10000;
        }

        int ret;
        try {
            ret = com.qiyi.xhook.NativeHandler.getInstance().start();
        } catch (Throwable ex) {
            ex.printStackTrace();
            Log.e("xhook", "xhook native start failed");
            ret = 10001;
        }
        return ret;
    }

    /**
     * Stop the hook mechanism.
     * @return 0 if successful, false otherwise.
     */
    public synchronized int stop() {
        if(!inited) {
            return 10000;
        }

        int ret;
        try {
            ret = com.qiyi.xhook.NativeHandler.getInstance().stop();
        } catch (Throwable ex) {
            ex.printStackTrace();
            Log.e("xhook", "xhook native stop failed");
            ret = 10001;
        }
        return ret;
    }

    /**
     * Re-hook after System.loadLibrary() and System.load().
     * @return 0 if successful, false otherwise.
     */
    public synchronized int refresh() {
        if(!inited) {
            return 10000;
        }

        int ret;
        try {
            ret = com.qiyi.xhook.NativeHandler.getInstance().refresh();
        } catch (Throwable ex) {
            ex.printStackTrace();
            Log.e("xhook", "xhook native refresh failed");
            ret = 10001;
        }
        return ret;
    }
}

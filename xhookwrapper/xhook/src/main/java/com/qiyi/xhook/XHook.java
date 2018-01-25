package com.qiyi.xhook;

import android.content.Context;
import android.util.Log;

/**
 * Created by caikelun on 18/01/2018.
 */

public class XHook {
    private static final XHook ourInstance = new XHook();

    public static XHook getInstance() {
        return ourInstance;
    }

    private XHook() {
    }

    public synchronized void init(Context ctx) {
        try {
            System.loadLibrary("xhook");
        } catch (Throwable e) {
            try {
                System.load(ctx.getFilesDir().getParent() + "/lib/libxhook.so");
            } catch (Throwable ex) {
                ex.printStackTrace();
                Log.e("xhook", "load libxhook.so failed");
                return;
            }
        }
    }

    public synchronized void refresh() {
        try {
            com.qiyi.xhook.NativeHandler.getInstance().refresh();
        } catch (Throwable ex) {
            ex.printStackTrace();
            Log.e("xhook", "xhook native start failed");
            return;
        }
    }
}

package com.qiyi.biz;

/**
 * Created by caikelun on 18/01/2018.
 */

public class Biz {
    private static final Biz ourInstance = new Biz();

    public static Biz getInstance() {
        return ourInstance;
    }

    private Biz() {
    }

    public synchronized void init() {
        System.loadLibrary("biz");
    }

    public synchronized void start() {
        com.qiyi.biz.NativeHandler.getInstance().start();
    }
}

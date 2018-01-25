package com.qiyi.test;

/**
 * Created by caikelun on 18/01/2018.
 */

public class Test {
    private static final Test ourInstance = new Test();

    public static Test getInstance() {
        return ourInstance;
    }

    private Test() {
    }

    public synchronized void init() {
        System.loadLibrary("test");
    }

    public synchronized void start() {
        com.qiyi.test.NativeHandler.getInstance().start();
    }
}

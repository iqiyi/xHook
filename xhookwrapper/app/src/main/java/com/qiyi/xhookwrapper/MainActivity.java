package com.qiyi.xhookwrapper;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;

public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        //init the target lib
        com.qiyi.test.Test.getInstance().init();

        //init xhook
        com.qiyi.xhook.XHook.getInstance().init(this.getApplicationContext());
        if(!com.qiyi.xhook.XHook.getInstance().isInited()) {
            return;
        }
        com.qiyi.xhook.XHook.getInstance().enableDebug(true); //default is false
        //com.qiyi.xhook.XHook.getInstance().enableSystemHook(true); //default is false
        //com.qiyi.xhook.XHook.getInstance().enableReldynHook(true); //default is false

        //init your biz lib
        com.qiyi.biz.Biz.getInstance().init();

        //target lib running
        com.qiyi.test.Test.getInstance().start();

        //for debug, get a chance to the target lib to run before hooked
        //you can compare the different before and after hooking
        try {
            Thread.sleep(200);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }

        //register hook points
        com.qiyi.biz.Biz.getInstance().start();

        //do start (and hook)
        com.qiyi.xhook.XHook.getInstance().start();

        //do refresh for some reason, maybe called after some System.loadLibrary()
        new Thread(new Runnable() {
            @Override
            public void run() {
                while(true)
                {
                    com.qiyi.xhook.XHook.getInstance().refresh();

                    try {
                        Thread.sleep(5000);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                }
            }
        }).start();
    }
}

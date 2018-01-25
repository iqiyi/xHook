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

        //init xhook lib and xhook-user lib
        com.qiyi.xhook.XHook.getInstance().init(this.getApplicationContext());
        com.qiyi.biz.Biz.getInstance().init();

        //target lib running
        com.qiyi.test.Test.getInstance().start();

        try {
            Thread.sleep(200);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }

        //register hook points
        com.qiyi.biz.Biz.getInstance().hook();

        //do refresh
        com.qiyi.xhook.XHook.getInstance().refresh();

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

package com.qiyi.xhookwrapper;

import androidx.appcompat.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;

public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        //load xhook
        com.qiyi.xhook.XHook.getInstance().init(this.getApplicationContext());
        if(!com.qiyi.xhook.XHook.getInstance().isInited()) {
            return;
        }
        //com.qiyi.xhook.XHook.getInstance().enableDebug(true); //default is false
        //com.qiyi.xhook.XHook.getInstance().enableSigSegvProtection(false); //default is true

        //load and run your biz lib (for register hook points)
        com.qiyi.biz.Biz.getInstance().init();
        com.qiyi.biz.Biz.getInstance().start();

        //xhook do refresh
        com.qiyi.xhook.XHook.getInstance().refresh(false);

        //load and run the target lib
        com.qiyi.test.Test.getInstance().init();
        com.qiyi.test.Test.getInstance().start();
        try {
            Thread.sleep(200);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }

        //xhook do refresh again
        com.qiyi.xhook.XHook.getInstance().refresh(false);

        //xhook do refresh again for some reason,
        //maybe called after some System.loadLibrary() and System.load()
        //*
        new Thread(new Runnable() {
            @Override
            public void run() {
                while(true)
                {
                    com.qiyi.xhook.XHook.getInstance().refresh(true);

                    try {
                        Thread.sleep(5000);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                }
            }
        }).start();
        //*/
    }
}

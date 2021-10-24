package com.dodola.jvmti;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;

import com.dodola.jvmtilib.JVMTIHelper;

public class MainActivity extends Activity {

    public class RunThread extends Thread {
        public  void  run() {
            int i = 0;
            while (i < 10) {
//                Float[] wordList = new Float[10];
                i += 1;
                i = i / 10;
            }

        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

//        JVMTIHelper.init(MainActivity.this);

        Button tv = findViewById(R.id.sample_text);
        tv.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                JVMTIHelper.init(MainActivity.this);
            }
        });
        findViewById(R.id.button_gc).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                System.gc();
                System.runFinalization();
            }
        });
        findViewById(R.id.button_modify_class).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                // JVMTIHelper.retransformClasses(new Class[]{Activity.class});
                for (int i = 0; i < 1000; i++) {
                    Float[] wordList = new Float[10];
                }
            }
        });
        findViewById(R.id.button_start_activity).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                RunThread thread = new RunThread();
                thread.start();
//                for (int i = 0; i < 1000000; i++) {
//                    Float[] wordList = new Float[10];
//                }

                    // hello();
//                getIsArtInUse();


//                startActivity(new Intent(MainActivity.this, Main2Activity.class));
            }
        });
    }
    public void hello() {
        System.gc();
        System.runFinalization();
    }
    private boolean getIsArtInUse() {
        final String vmVersion = System.getProperty("java.vm.version");
        System.out.print("get version");
//        Log.d("get version", "_____________________" + vmVersion);
        String vm = System.getProperty("java.vm.name") + " " + System.getProperty("java.vm.version");
        Log.d("get version", "_____________________" + vm);
        return vmVersion != null && vmVersion.startsWith("2");
    }

}


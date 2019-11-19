package com.diaochan.playerdemo;

import android.os.Bundle;
import android.os.Environment;
import android.view.SurfaceView;
import android.view.View;
import android.view.WindowManager;

import androidx.appcompat.app.AppCompatActivity;

import java.io.File;

public class MainActivity extends AppCompatActivity {


    private SurfaceView mSurfaceView;
    private WangyiPlayer player;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        //设置全屏
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON, 
                WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        mSurfaceView = findViewById(R.id.surfaceView);
        player = new WangyiPlayer();
        player.setSurfaceView(mSurfaceView);
        
    }


    public void open(View view) {
        File file = new File(Environment.getExternalStorageDirectory(),"input.mp4");
        player.start(file.getAbsolutePath());
    }
}

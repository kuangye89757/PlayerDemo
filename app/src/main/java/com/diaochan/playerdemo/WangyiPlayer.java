package com.diaochan.playerdemo;

import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

/**
 * 播放器
 * @author diaochan
 */
public class WangyiPlayer implements SurfaceHolder.Callback {
    static {
        System.loadLibrary("wangyiplayer");
    }

    private SurfaceHolder mSurfaceHolder;

    /**
     * 设置监听
     * @param surfaceView
     */
    public void setSurfaceView(SurfaceView surfaceView){
        if(mSurfaceHolder != null){
            mSurfaceHolder.removeCallback(this);
        }
        mSurfaceHolder = surfaceView.getHolder();
        mSurfaceHolder.addCallback(this);
    }
    
    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        mSurfaceHolder = holder;
        
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {

    }

    public void start(String path) {
        native_start(path,mSurfaceHolder.getSurface());
    }

    public native void native_start(String path, Surface surface);


}

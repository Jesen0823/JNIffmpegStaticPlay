package com.example.jniffmpegstaticplay;

import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import androidx.annotation.NonNull;

public class StaticPlayer implements SurfaceHolder.Callback {
    // Used to load the 'jniffmpegstaticplay' library on application startup.
    static {
        System.loadLibrary("jniffmpegstaticplay");
    }

    private SurfaceHolder surfaceHolder;

    public void setSurfaceView(SurfaceView surfaceView){
        if (null != this.surfaceHolder){
            this.surfaceHolder.removeCallback(this);
        }
        this.surfaceHolder = surfaceView.getHolder();
        this.surfaceHolder.addCallback(this);
    }

    @Override
    public void surfaceCreated(@NonNull SurfaceHolder surfaceHolder) {

    }

    @Override
    public void surfaceChanged(@NonNull SurfaceHolder surfaceHolder, int i, int i1, int i2) {
        this.surfaceHolder = surfaceHolder;
    }

    @Override
    public void surfaceDestroyed(@NonNull SurfaceHolder surfaceHolder) {

    }

    public void start(String path) {
        native_start(path, surfaceHolder.getSurface());
        Log.d("XXX","path = "+path);
    }

    public native void native_start(String path, Surface surface);
}

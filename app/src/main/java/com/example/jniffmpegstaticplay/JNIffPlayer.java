package com.example.jniffmpegstaticplay;

import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import androidx.annotation.NonNull;

import com.example.jniffmpegstaticplay.utils.OLog;

public class JNIffPlayer implements SurfaceHolder.Callback {

    static {
        System.loadLibrary("jniffmpegstaticplay");
    }

    private SurfaceHolder surfaceHolder;
    private String dataSource;

    public void setSurfaceView(SurfaceView surfaceView){
        if (null != this.surfaceHolder){
            this.surfaceHolder.removeCallback(this);
        }
        this.surfaceHolder = surfaceView.getHolder();
        this.surfaceHolder.addCallback(this);
    }

    @Override
    public void surfaceCreated(@NonNull SurfaceHolder surfaceHolder) {
        OLog.d("MainActivity, surfaceCreated");
        native_set_surface(surfaceHolder.getSurface());
    }

    @Override
    public void surfaceChanged(@NonNull SurfaceHolder surfaceHolder, int i, int i1, int i2) {
        this.surfaceHolder = surfaceHolder;
    }

    @Override
    public void surfaceDestroyed(@NonNull SurfaceHolder surfaceHolder) {

    }

    public void prepare(){
        native_prepare(dataSource);
    }

    public void setDataSource(String abPath) {
        this.dataSource = abPath;
    }

    // 播放器准备的监听回调
    public void onPrepareFromJNI() {
        if (null != prepareListener) {
            prepareListener.onPrepare();
        }
    }

    // 播放进度的监听回调
    public void onProgressFromJNI(int progress) {
        if (null != progressListener) {
            progressListener.onProgress(progress);
        }
    }

    // 播放错误监听回调
    public void onErrorFromJNI(int errorCode) {
        if (null != errorListener) {
            errorListener.onError(errorCode);
        }
    }

    public void start() {
        native_start();
    }

    public interface OnPrepareListener{
        void onPrepare();
    }

    public interface OnProgressListener{
        void onProgress(int progress);
    }

    public interface OnErrorListener{
        void onError(int errorCode);
    }

    private OnPrepareListener prepareListener;
    private OnProgressListener progressListener;
    private OnErrorListener errorListener;

    public void setOnPrepareListener(OnPrepareListener prepareListener) {
        this.prepareListener = prepareListener;
    }

    public void setOnProgressListener(OnProgressListener progressListener) {
        this.progressListener = progressListener;
    }

    public void setOnErrorListener(OnErrorListener errorListener) {
        this.errorListener = errorListener;
    }

    private native void native_prepare(String dataSource);
    private native void native_start();
    private native void native_set_surface(Surface surface);


}

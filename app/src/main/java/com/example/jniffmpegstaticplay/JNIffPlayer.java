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

    //准备过程错误码
    public static final int ERROR_CODE_FFMPEG_PREPARE = 1000;
    //播放过程错误码
    public static final int ERROR_CODE_FFMPEG_PLAY = 2000;
    //打不开视频
    public static final int FFMPEG_CAN_NOT_OPEN_URL = (ERROR_CODE_FFMPEG_PREPARE - 1);
    //找不到媒体流信息
    public static final int FFMPEG_CAN_NOT_FIND_STREAMS = (ERROR_CODE_FFMPEG_PREPARE - 2);
    //找不到解码器
    public static final int FFMPEG_FIND_DECODER_FAIL = (ERROR_CODE_FFMPEG_PREPARE - 3);
    //无法根据解码器创建上下文
    public static final int FFMPEG_ALLOC_CODEC_CONTEXT_FAIL = (ERROR_CODE_FFMPEG_PREPARE - 4);
    //根据流信息 配置上下文参数失败
    public static final int FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL = (ERROR_CODE_FFMPEG_PREPARE - 5);
    //打开解码器失败
    public static final int FFMPEG_OPEN_DECODER_FAIL = (ERROR_CODE_FFMPEG_PREPARE - 6);
    //没有音视频
    public static final int FFMPEG_NOMEDIA = (ERROR_CODE_FFMPEG_PREPARE - 7);
    //读取媒体数据包失败
    public static final int FFMPEG_READ_PACKETS_FAIL = (ERROR_CODE_FFMPEG_PLAY - 1);
    private static final String TAG = "JNIffPlayer";

    private SurfaceHolder surfaceHolder;
    private String dataSource;

    public void setSurfaceView(SurfaceView surfaceView) {
        if (null != this.surfaceHolder) {
            this.surfaceHolder.removeCallback(this);
        }
        this.surfaceHolder = surfaceView.getHolder();
        this.surfaceHolder.addCallback(this);
    }

    private OnPrepareListener prepareListener;
    private OnProgressListener progressListener;
    private OnErrorListener errorListener;

    // 设置播放源
    public void setDataSource(String abPath) {
        this.dataSource = abPath;
        OLog.d(TAG + ", setDataSource: " + abPath);

    }

    // 播放准备
    public void prepare() {
        OLog.d(TAG + ", prepare");
        native_prepare(dataSource);
    }

    // 启动播放
    public void start() {
        OLog.d(TAG + ", start");
        native_start();
    }

    // 获取视频总时长
    public int getDuration() {
        OLog.d(TAG + ", getDuration");

        return native_get_duration();
    }

    // 快进快退
    public void seekTo(int seekPoint) {
        OLog.d(TAG + ", seekTo :" + seekPoint);
        if (seekPoint < 0) {
            OLog.d(TAG + "seek to " + seekPoint + " ia a error!");
            return;
        }
        native_seek(seekPoint);
    }

    // 暂停
    public void pause() {
        native_pause();
    }

    // 恢复暂停
    public void resume() {
        native_resume();
    }

    // 停止播放
    public void stop() {
        OLog.d(TAG + ", stop");
        native_stop();
    }

    // 释放播放器
    public void release() {
        OLog.d(TAG + ", release");
        surfaceHolder.removeCallback(this);
        native_release();
    }

    @Override
    public void surfaceCreated(@NonNull SurfaceHolder surfaceHolder) {
        OLog.d(TAG + ", surfaceCreated");
    }

    @Override
    public void surfaceChanged(@NonNull SurfaceHolder surfaceHolder, int i, int i1, int i2) {
        OLog.d(TAG + ", surfaceChanged");
        this.surfaceHolder = surfaceHolder;
        native_set_surface(surfaceHolder.getSurface());
    }

    @Override
    public void surfaceDestroyed(@NonNull SurfaceHolder surfaceHolder) {
        OLog.d(TAG + ", surfaceDestroyed");

    }

    // 播放器准备的监听回调
    public void onPrepareFromJNI() {
        if (null != prepareListener) {
            prepareListener.onPrepared();
        }
    }

    public interface OnProgressListener{
        void onProgress(int progress);
    }

    public interface OnErrorListener{
        void onError(int errorCode);
    }

    public void setOnPrepareListener(OnPrepareListener prepareListener) {
        this.prepareListener = prepareListener;
    }

    public void setOnProgressListener(OnProgressListener progressListener) {
        this.progressListener = progressListener;
    }

    public void setOnErrorListener(OnErrorListener errorListener) {
        this.errorListener = errorListener;
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

    private native void native_set_surface(Surface surface);

    private native int native_get_duration();

    private native void native_prepare(String dataSource);

    private native void native_start();

    private native void native_seek(int seekPoint);

    private native void native_pause();

    private native void native_resume();

    private native void native_stop();

    private native void native_release();

    public interface OnPrepareListener {
        void onPrepared();
    }

}

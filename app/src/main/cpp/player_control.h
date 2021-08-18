//
// Created by X1 Carbon on 2021/8/15.
// 控制层

#ifndef JNIFFMPEGSTATICPLAY_PLAYER_CONTROL_H
#define JNIFFMPEGSTATICPLAY_PLAYER_CONTROL_H

#include <pthread.h>
#include <android/native_window_jni.h>
#include "call_java_helper.h"
#include "audio_channel.h"
#include "video_channel.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/time.h>
};

class PlayerControl {
public:
    PlayerControl(CallJavaHelper *callJavaHelper, const char *path);

    void prepare();

    void prepareControl();

    ~PlayerControl();

    void start();

    void play();

    void setRenderFrameCallback(RenderFrameCallback renderCallback);

    int get_duration() const;

    void seekTo(int point);

    void pause();

    void resume();

    void stop();


    AudioChannel *audioChannel = 0;
    VideoChannel *videoChannel = 0;
    int isPlaying;
    pthread_t pid_prepare;
    AVFormatContext *formatContext;

private:
    // 准备线程
    pthread_t pid_dec_play; // 解码线程运行到播放结束
    pthread_t pid_stop; // 停止播放
    char *url;
    CallJavaHelper *callJavaHelper;
    RenderFrameCallback frameCallback;
    int duration = 0;
    // 锁,利用线程间共享的全局变量进行同步的一种机制, 参考：https://blog.csdn.net/lyx_323/article/details/82897192
    pthread_mutex_t seekMutex;
};

#endif //JNIFFMPEGSTATICPLAY_PLAYER_CONTROL_H

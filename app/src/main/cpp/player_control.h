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

private:
    int isPlaying;
    pthread_t pid_prepare; // 准备线程
    pthread_t pid_dec_play; // 解码线程运行到播放结束
    AVFormatContext *formatContext;
    char *url;
    CallJavaHelper *callJavaHelper;

    AudioChannel *audioChannel = 0;
    VideoChannel *videoChannel = 0;
    RenderFrameCallback frameCallback;
};

#endif //JNIFFMPEGSTATICPLAY_PLAYER_CONTROL_H

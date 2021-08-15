//
// Created by X1 Carbon on 2021/8/15.
// 控制层

#ifndef JNIFFMPEGSTATICPLAY_PLAYER_CONTROL_H
#define JNIFFMPEGSTATICPLAY_PLAYER_CONTROL_H

#include <pthread.h>
#include <android/native_window_jni.h>

extern "C"{
#include <libavformat/avformat.h>
#include <libavutil/time.h>
};
 class PlayerControl {
 public:
     PlayerControl(const char *path);

     void prepare();

     void prepareControl();

     ~PlayerControl();

private:
 pthread_t pid_prepare; // 准备线程
 AVFormatContext * formatContext;
 char *url;
};

#endif //JNIFFMPEGSTATICPLAY_PLAYER_CONTROL_H

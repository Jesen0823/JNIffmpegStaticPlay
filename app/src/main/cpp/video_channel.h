//
// Created by X1 Carbon on 2021/8/16.
//

#ifndef JNIFFMPEGSTATICPLAY_VIDEO_CHANNEL_H
#define JNIFFMPEGSTATICPLAY_VIDEO_CHANNEL_H

#include "base_channel.h"
#include "audio_channel.h"
#include "call_java_helper.h"
#include <pthread.h>
#include <android//native_window.h>

/** 用来传递解码并转换后的ARGB
 * int 像素个数
 * int 图片的宽度
 * int 图片高度
 */
typedef void (*RenderFrameCallback)(uint8_t *, int, int, int);

class VideoChannel : public BaseChannel {

public:
    VideoChannel(int id, CallJavaHelper *callJavaHelper, AVCodecContext *codecContext);

    ~VideoChannel();

    virtual void play();

    virtual void stop();

    void decodePacket();

    void setRenderFrameCallback(RenderFrameCallback renderCallback);

private:
    pthread_t pid_video_decode;
    pthread_t pid_video_play;
    RenderFrameCallback renderCallback;
};

#endif //JNIFFMPEGSTATICPLAY_VIDEO_CHANNEL_H

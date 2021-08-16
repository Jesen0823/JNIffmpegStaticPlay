//
// Created by X1 Carbon on 2021/8/16.
//

#ifndef JNIFFMPEGSTATICPLAY_VIDEO_CHANNEL_H
#define JNIFFMPEGSTATICPLAY_VIDEO_CHANNEL_H

#include "base_channel.h"

class VideoChannel: public BaseChannel{

public:
    VideoChannel(int id, CallJavaHelper *callJavaHelper, AVCodecContext *codecContext);

    ~VideoChannel();

    virtual void play();
    virtual void stop();
};

#endif //JNIFFMPEGSTATICPLAY_VIDEO_CHANNEL_H

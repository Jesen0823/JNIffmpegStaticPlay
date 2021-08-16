//
// Created by X1 Carbon on 2021/8/16.
//

#ifndef JNIFFMPEGSTATICPLAY_AUDIO_CHANNEL_H
#define JNIFFMPEGSTATICPLAY_AUDIO_CHANNEL_H

#include "base_channel.h"

class AudioChannel: public BaseChannel{
public:

    AudioChannel(int index, CallJavaHelper * callJavaHelper, AVCodecContext * codecContext);

    ~AudioChannel();

    virtual void play();
    virtual void stop();
};

#endif //JNIFFMPEGSTATICPLAY_AUDIO_CHANNEL_H

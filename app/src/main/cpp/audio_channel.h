//
// Created by X1 Carbon on 2021/8/16.
//

#ifndef JNIFFMPEGSTATICPLAY_AUDIO_CHANNEL_H
#define JNIFFMPEGSTATICPLAY_AUDIO_CHANNEL_H

class AudioChannel{
public:

    AudioChannel(int index, CallJavaHelper * callJavaHelper, AVCodecContext * codecContext);

    ~AudioChannel();
private:

};

#endif //JNIFFMPEGSTATICPLAY_AUDIO_CHANNEL_H

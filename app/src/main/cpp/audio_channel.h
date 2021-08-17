//
// Created by X1 Carbon on 2021/8/16.
//

#ifndef JNIFFMPEGSTATICPLAY_AUDIO_CHANNEL_H
#define JNIFFMPEGSTATICPLAY_AUDIO_CHANNEL_H

#include "base_channel.h"
#include "safe_queue.h"
#include <SLES/OpenSLES_Android.h>

extern "C" {
#include <libswresample/swresample.h>
};

class AudioChannel : public BaseChannel {
public:

    AudioChannel(int index, CallJavaHelper *callJavaHelper, AVCodecContext *codecContext,
                 AVRational time_base);

    ~AudioChannel();

    virtual void play();

    virtual void stop();

    void initOpenSL();

    void decodeAudio();

    int getPCM();

    uint8_t *out_pcm_buffers;
    int out_buffers_size;

private:
    // 音频引擎
    SLObjectItf engineObj = NULL;
    // 音频对象
    SLEngineItf engineInterface = NULL;
    // 混音器
    SLObjectItf outputMixObj = NULL;
    // 播放器
    SLObjectItf bqPlayerObj = NULL;
    // 回调接口
    SLPlayItf bqPlayerInterface = NULL;
    // 缓冲队列
    SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue = NULL;

    pthread_t pid_init_opensl_play;
    pthread_t pid_audio_decode;

    SwrContext *swrContext = 0;
    int out_channels;   // 声道数
    int out_sample_size; // 采样位数
    int out_sample_rate; // 采样率
};

#endif //JNIFFMPEGSTATICPLAY_AUDIO_CHANNEL_H

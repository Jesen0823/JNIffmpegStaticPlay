//
// Created by X1 Carbon on 2021/8/16.
//

#ifndef JNIFFMPEGSTATICPLAY_BASE_CHANNEL_H
#define JNIFFMPEGSTATICPLAY_BASE_CHANNEL_H

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/frame.h>
#include <libavutil/time.h>
};
#include "safe_queue.h"
#include "call_java_helper.h"
#include "player_control.h"

class BaseChannel{
public:

    SafeQueue<AVPacket *> pkt_queue; // packet队列
    SafeQueue<AVFrame *> frame_queue; // Frame队列
    volatile int channelId;
    volatile bool isPlaying;
    AVCodecContext *avCodecContext;
    CallJavaHelper *callJavaHelper;

    BaseChannel(volatile int channelId, AVCodecContext *avCodecContext,
                CallJavaHelper *callJavaHelper);


    virtual ~BaseChannel(){
        if (avCodecContext){
            avcodec_close(avCodecContext);
            avcodec_free_context(&avCodecContext);
            avCodecContext = 0;
        }
        pkt_queue.clear();
        frame_queue.clear();
        LOGE("free channel: packet queue size %d, frame queue size %d",pkt_queue.size(), frame_queue.size());
    }

    // 虚方法，类似java的abstract
    virtual void play() = 0;
    virtual void stop() = 0;

};

#endif //JNIFFMPEGSTATICPLAY_BASE_CHANNEL_H
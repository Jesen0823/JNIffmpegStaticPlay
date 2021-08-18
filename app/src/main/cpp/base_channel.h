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

#include "call_java_helper.h"
#include "safe_queue.h"

class BaseChannel {
public:
    BaseChannel(int id, CallJavaHelper *callJavaHelper, AVCodecContext *codecContext,
                AVRational time_base) : channelId(id),
                                        callJavaHelper(callJavaHelper),
                                        codecContext(codecContext),
                                        time_base(time_base) {

        pkt_queue.setReleaseCallback(releaseAVPacket);
        frame_queue.setReleaseCallback(releaseAVFrame);
    }

    SafeQueue<AVPacket *> pkt_queue; // packet队列
    SafeQueue<AVFrame *> frame_queue; // Frame队列
    volatile int channelId;
    volatile bool isPlaying = 0;
    volatile bool isPause = 0;

    AVCodecContext *codecContext;
    CallJavaHelper *callJavaHelper;
    AVRational time_base; // 时间基，一种由分子分母表示的刻度单位
    double syn_clock = 0; // 同步时钟，记录音频播放时间线，时间线是指相对时间

    virtual ~BaseChannel() {
        if (codecContext) {
            avcodec_close(codecContext);
            avcodec_free_context(&codecContext);
            codecContext = 0;
        }
        clearQueue();
        LOGE("free channel: packet queue size %d, frame queue size %d", pkt_queue.size(),
             frame_queue.size());
    }

    // 虚方法，类似java的abstract
    virtual void play() = 0;

    virtual void pause() = 0;

    virtual void resume() = 0;

    virtual void stop() = 0;

    static void releaseAVPacket(AVPacket **packet) {
        if (packet) {
            av_packet_free(packet);
            *packet = 0;
        }
    }

    static void releaseAVFrame(AVFrame **frame) {
        if (frame) {
            av_frame_free(frame);
            *frame = 0;
        }
    }

    void startWork() {
        pkt_queue.setWork(1);
        frame_queue.setWork(1);
    }

    void stopWork() {
        pkt_queue.setWork(0);
        frame_queue.setWork(0);
    }

    void clearQueue() {
        pkt_queue.clear();
        frame_queue.clear();
    }
};

#endif //JNIFFMPEGSTATICPLAY_BASE_CHANNEL_H

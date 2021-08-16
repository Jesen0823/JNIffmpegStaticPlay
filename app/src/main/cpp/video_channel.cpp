//
// Created by X1 Carbon on 2021/8/16.
//

#include "video_channel.h"
#include "macro.h"
#include "call_java_helper.h"

extern "C" {
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
}

VideoChannel::VideoChannel(int id, CallJavaHelper *callJavaHelper, AVCodecContext *codecContext)
        : BaseChannel(id, callJavaHelper, avCodecContext) {

}

VideoChannel::~VideoChannel() {

}

void *decode(void *args) {
    VideoChannel *videoChannel = static_cast<VideoChannel *>(args);
    videoChannel->decodePacket();
    return 0;
}

void *syn_play(void *args) {
    VideoChannel *videoChannel = static_cast<VideoChannel *>(args);
    videoChannel->syn_frame_play();
    return 0;
}

void VideoChannel::play() {
    pkt_queue.setWork(1);
    frame_queue.setWork(1);
    isPlaying = true;

    pthread_create(&pid_video_decode, NULL, decode, this);
    pthread_create(&pid_video_play, NULL, syn_play, this);
}

void VideoChannel::stop() {

}

void VideoChannel::decodePacket() {
    AVPacket *packet = 0;
    while (isPlaying) {
        int ret = pkt_queue.pop(packet);
        if (!isPlaying) {
            break;
        }
        if (!ret) {
            continue;
        }
        ret = avcodec_send_packet(avCodecContext, packet);
        releaseAVPacket(packet);
        if (ret == AVERROR(EAGAIN)) {
            continue;
        } else if (ret < 0) {
            break;
        }


        AVFrame *frame = av_frame_alloc();
        ret = avcodec_receive_frame(avCodecContext, frame);
        // packet是压缩数据，需要解压
        frame_queue.put(frame);
        while (frame_queue.size() > 100 && isPlaying) {
            av_usleep(1000 * 10);
            continue;
        }
    }
    releaseAVPacket(packet); // 防止有未释放packet
}

void VideoChannel::setRenderFrameCallback(RenderFrameCallback renderCallback) {
    this->renderCallback = renderCallback;
}


// 播放
void VideoChannel::syn_frame_play() {

    //要对原始数据进行格式转换：yuv > rgba
    SwsContext *sws_ctx = sws_getContext(codecContext->width, codecContext->height,
                                         codecContext->pix_fmt, codecContext->width,
                                         codecContext->height,
                                         AV_PIX_FMT_RGBA, SWS_BILINEAR, NULL, NULL, NULL);
    // ARGB数据
    uint8_t *dst_data[4];
    int dst_linesize[4];
    // 给dst_data dst_linesize 申请内存
    av_image_alloc(dst_data, dst_linesize,
                   codecContext->width, codecContext->height, AV_PIX_FMT_RGBA, 1);

    //根据fps（传入的流的平均帧率来控制每一帧的延时时间）
    double delay_time_per_frame = 1.0 / fps;

    AVFrame *frame = 0;
    while (isPlaying) {
        int ret = frame_queue.pop(frame);
        if (!isPlaying) {
            //如果停止播放了，跳出循环 释放packet
            break;
        }
        if (!ret) {
            //取数据包失败
            continue;
        }
        /**
         * 取到了yuv原始数据rame->data，行格式转换
         * 第二个参数，片数组slice[] = 一帧；slice(片) = 片头 + n * 宏块
         * linesize 是一行像素数据，片的长度
         * dst_data 转换出的ARGB数据
         * linesize 一行的像素数，即图片宽度
         * */
        sws_scale(sws_ctx,
                  reinterpret_cast<const uint8_t *const *>(frame->data),
                  frame->linesize, 0, frame->height, dst_data, dst_linesize);

        //进行休眠
        //每一帧还有自己的额外延时时间
        //extra_delay = repeat_pict / (2*fps)
        double extra_delay = frame->repeat_pict / (2 * fps);
        double real_delay = delay_time_per_frame + extra_delay;
        //单位是：微秒
        av_usleep(real_delay * 1000000);

        /**
         * 回调出去,去native-lib里渲染
         * data： 图像内容，AV_PIX_FMT_RGBA格式的数据
         * linesize
         * */
        renderCallback(dst_data[0], dst_linesize[0], codecContext->width, codecContext->height);
        releaseAVFrame(&frame);
    }
    releaseAVFrame(&frame);
    isPlaying = 0;
    av_freep(&dst_data[0]);
    sws_freeContext(sws_ctx);
}
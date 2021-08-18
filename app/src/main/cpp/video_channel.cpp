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

/**
 * 主动丢帧：Packet
 * 如果丢packet队列,要防止丢掉关键帧,否则会引起花屏
 * */
void dropPacket(queue<AVPacket *> &q) {
    while (!q.empty()) {
        LOGD("video_cahannel,丢掉视频Packet帧。。");
        AVPacket *pkt = q.front();
        if (pkt->flags != AV_PKT_FLAG_KEY) {
            q.pop();
            BaseChannel::releaseAVPacket(&pkt);
        } else {
            break;
        }
    }
}

/**
 * 主动丢帧：Frame
 * 比丢Packet更好
 * */
void dropFrame(queue<AVFrame *> &q) {
    while (!q.empty()) {
        LOGD("video_cahannel,丢掉视频Frame帧。。");
        AVFrame *frame = q.front();
        q.pop();
        BaseChannel::releaseAVFrame(&frame);
    }
}

VideoChannel::VideoChannel(int id, CallJavaHelper *callJavaHelper, AVCodecContext *codecContext,
                           AVRational time_base)
        : BaseChannel(id, callJavaHelper, codecContext, time_base) {

    // 设置音视频同步时的丢帧策略：丢Packet还是Frame?
    frame_queue.setReleaseCallback(releaseAVFrame);
    frame_queue.setSyncHandle(dropFrame);

    pthread_mutex_init(&v_mutex, NULL);
    pthread_cond_init(&v_cond, NULL);
}

VideoChannel::~VideoChannel() {
    pthread_cond_destroy(&v_cond);
    pthread_mutex_destroy(&v_mutex);
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
    startWork();
    isPlaying = true;

    pthread_create(&pid_video_decode, NULL, decode, this);
    pthread_create(&pid_video_play, NULL, syn_play, this);
}

void VideoChannel::pause() {
    isPause = 1;
}

void VideoChannel::resume() {
    isPause = 0;
    pthread_cond_signal(&v_cond);
}

void VideoChannel::stop() {
    isPlaying = 0;
    callJavaHelper = 0;
    stopWork();
    pthread_join(pid_video_decode, 0);
    pthread_join(pid_video_play, 0);
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

        ret = avcodec_send_packet(codecContext, packet);
        releaseAVPacket(&packet);
        if (ret == AVERROR(EAGAIN)) {
            continue;
        } else if (ret < 0) {
            break;
        }

        AVFrame *frame = av_frame_alloc();
        ret = avcodec_receive_frame(codecContext, frame);
        // packet是压缩数据，需要解压
        frame_queue.push(frame);
        while (frame_queue.size() > 100 && isPlaying) {
            av_usleep(1000 * 10);
            continue;
        }
    }
    releaseAVPacket(&packet); // 防止有未释放packet
}

void VideoChannel::setRenderFrameCallback(RenderFrameCallback callback) {
    this->renderCallback = callback;
}


// 播放
void VideoChannel::syn_frame_play() {

    pthread_mutex_lock(&v_mutex);
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

    AVFrame *frame = 0;
    while (isPlaying) {

        if (isPause) {
            pthread_cond_wait(&v_cond, &v_mutex);
        }

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

        /************************ 音视频同步 *********************/
        LOGD("decode a frame size:%d", frame_queue.size());
        // 视频每一帧播放时间,也就是现在距下一帧播放时间,但是并没有把解码时间算进去
        double frame_delay = 1.0 / fps;
        // 再加上解码耗时时间
        double real_delay = frame_delay + frame->repeat_pict / (2 * fps);
        // 拿到音频时钟，即音频帧当前显示时间，单位是time_base
        double audio_frame_clock = audioChannel->syn_clock;
        // 视频时钟，视频帧本来的显示时间
        //double video_frame_clock = frame->pts * av_q2d(time_base);
        double video_frame_clock = frame->best_effort_timestamp * av_q2d(time_base);

        if (!audioChannel) {
            // 没有音频流的情况
            av_usleep(real_delay * 1000000);
            if (callJavaHelper) {
                callJavaHelper->onProgress(THREAD_CHILD, video_frame_clock);
            }
        } else {
            // 视频时间线刻度 - 音频时间线刻度
            double diff = video_frame_clock - audio_frame_clock;
            LOGD("同步， 视频时间 - 音频时间 = 4%f", diff);

            if (diff > 0) { // 视频超前
                if (diff > 1) { // 相差比较大
                    av_usleep(real_delay * 2 * 100000);
                } else {
                    av_usleep(real_delay + diff * 1000000); // 单位是微秒
                }
            } else if (diff < 0) { // 音频超前
                if (fabs(diff) >= 0.05) {
                    //releaseAVFrame(&frame); // 丢掉当前帧
                    frame_queue.sync();
                    continue;
                }
            } else {
                LOGD("音视频同步，无需调整");
            }
        }
        releaseAVFrame(&frame);

        /**
         * 回调出去,去native-lib里渲染
         * data： 图像内容，AV_PIX_FMT_RGBA格式的数据
         * linesize
         * */
        renderCallback(dst_data[0], dst_linesize[0], codecContext->width, codecContext->height);
    }
    pthread_mutex_unlock(&v_mutex);
    releaseAVFrame(&frame);
    isPlaying = 0;
    av_freep(&dst_data[0]);
    sws_freeContext(sws_ctx);
}

void VideoChannel::setFps(int fps) {
    this->fps = fps;
}


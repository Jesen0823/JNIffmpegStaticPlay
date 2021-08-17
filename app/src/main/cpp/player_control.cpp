//
// Created by X1 Carbon on 2021/8/15.
//

#include "player_control.h"
#include "macro.h"

void *runPrepare(void *args) {
    PlayerControl *playerControl = static_cast<PlayerControl *> (args);
    //playerControl->prepare();
    playerControl->prepareControl();
    // 一定要返回0
    return 0;
}

PlayerControl::PlayerControl(CallJavaHelper *callJavaHelper, const char *path) {
    this->callJavaHelper = callJavaHelper;
    url = new char[strlen(path) + 1];
    strcpy(url, path);
    // 初始化seekMutex
    pthread_mutex_init(&seekMutex, 0);
}

PlayerControl::~PlayerControl() {
    DELETE(url);
    DELETE(callJavaHelper);
    pthread_mutex_destroy(&seekMutex);
}

void PlayerControl::prepare() {
    // 传递this对象为了run函数中playerControl可以调用成员方法prepare()
    pthread_create(&pid_prepare, NULL, runPrepare, this);
}

// 开始视频渲染,该方法中子线程可以访问对象的属性
void PlayerControl::prepareControl() {
    // 初始化网络模块
    avformat_network_init();
    formatContext = avformat_alloc_context();
    //1、打开URL
    AVDictionary *opts = NULL;
    //设置超时3秒
    av_dict_set(&opts, "timeout", "10000000", 0);
    //强制指定AVFormatContext中AVInputFormat的。这个参数一般情况下可以设置为NULL，这样FFmpeg可以自动检测AVInputFormat。
    //输入文件的封装格式
    //av_find_input_format("avi") // 可以是rtmp
    // ret为0 表示成功
    int ret = avformat_open_input(&formatContext, url, NULL, &opts);
    if (ret != 0) {
        LOGE("prepareControl open failed.");
        if (callJavaHelper) {
            callJavaHelper->onError(THREAD_CHILD, FFMPEG_CAN_NOT_OPEN_URL);
        }
        return;
    }
    //2 查找媒体中的流信息
    ret = avformat_find_stream_info(formatContext, 0);
    if (ret < 0) {
        LOGE("find info of stream failed：%s", av_err2str(ret));
        // 反射通知java
        if (callJavaHelper) {
            callJavaHelper->onError(THREAD_CHILD, FFMPEG_CAN_NOT_FIND_STREAMS);
        }
        return;
    }

    duration = formatContext->duration / AV_TIME_BASE;

    for (int index = 0; index < formatContext->nb_streams; ++index) {
        AVStream *stream = formatContext->streams[index];
        AVCodecParameters *codecpar = stream->codecpar;
        // 获取解码器
        AVCodec *dec = avcodec_find_decoder(codecpar->codec_id);
        if (!dec) {
            LOGE("find decoder failed.");
            if (callJavaHelper) {
                callJavaHelper->onError(THREAD_CHILD, FFMPEG_CAN_NOT_FIND_STREAMS);
            }
            return;
        }

        // 创建解码器上下文
        AVCodecContext *codecContext = avcodec_alloc_context3(dec);
        if (!codecContext) {
            LOGE("alloc coder context failed.");
            if (callJavaHelper) {
                callJavaHelper->onError(THREAD_CHILD, FFMPEG_ALLOC_CODEC_CONTEXT_FAIL);
            }
            return;
        }

        // 复制参数
        ret = avcodec_parameters_to_context(codecContext, codecpar);
        if (ret < 0) {
            LOGE("add params to codecContext failed.");
            if (callJavaHelper)
                callJavaHelper->onError(THREAD_CHILD, FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL);
            return;
        }
        // 打开解码器
        ret = avcodec_open2(codecContext, dec, 0);
        if (ret != 0) {
            LOGE("open coder failed.");
            if (callJavaHelper) {
                callJavaHelper->onError(THREAD_CHILD, FFMPEG_OPEN_DECODER_FAIL);
            }
            return;
        }

        // 处理音频视频流
        if (codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            AVRational frame_rate = stream->avg_frame_rate;
            int fps = av_q2d(frame_rate); // 就是 frame_rate.num / frame_rate.den;
            LOGD("player_control, 帧率: %d", fps);
            videoChannel = new VideoChannel(index, callJavaHelper, codecContext, stream->time_base);
            videoChannel->setRenderFrameCallback(frameCallback);
            videoChannel->setFps(fps);
        } else if (codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioChannel = new AudioChannel(index, callJavaHelper, codecContext, stream->time_base);
        }
    }

    //既没有音频也没有视频
    if (!audioChannel && !videoChannel) {
        LOGE("not find audio && video.");
        if (callJavaHelper) {
            callJavaHelper->onError(THREAD_CHILD, FFMPEG_NOMEDIA);
        }
        return;
    }

    videoChannel->audioChannel = audioChannel;

    //准备好了，反射通知java
    if (callJavaHelper) {
        LOGE("parpare and init success.");
        callJavaHelper->onParpare(THREAD_CHILD);
    }
}

void *start_dec_play_thread(void *args) {
    PlayerControl *playerControl = static_cast<PlayerControl *>(args);
    playerControl->play();
    return 0;
}

void PlayerControl::start() {
    isPlaying = true;
    if (audioChannel) {
        audioChannel->play();
    }
    if (videoChannel) {
        videoChannel->play();
    }
    pthread_create(&pid_dec_play, NULL, start_dec_play_thread, this);
}

// 真正的解码视频
void PlayerControl::play() {
    int ret = 0;
    while (isPlaying) {
        // 防止队列阻塞
        if (audioChannel && audioChannel->pkt_queue.size() > 100) {
            // 生产速度大于消费，休眠10ms
            av_usleep(1000 * 10);
            continue;
        }
        if (videoChannel && videoChannel->pkt_queue.size() > 100) {
            // 生产速度大于消费，休眠10ms
            av_usleep(1000 * 10);
            continue;
        }

        // 读取数据包
        AVPacket *packet = av_packet_alloc();
        pthread_mutex_lock(&seekMutex);
        ret = av_read_frame(formatContext, packet);
        pthread_mutex_unlock(&seekMutex);
        if (ret == 0) {
            // 将数据包加入队列
            if (audioChannel && packet->stream_index == audioChannel->channelId) {
                audioChannel->pkt_queue.push(packet);
            } else if (videoChannel && packet->stream_index == videoChannel->channelId) {
                videoChannel->pkt_queue.push(packet);
            }
        } else if (ret == AVERROR_EOF) {
            //表示读完了，但不一定播放完毕
            //要考虑读完了，是否播完了的情况
            if (videoChannel->pkt_queue.empty() && videoChannel->frame_queue.empty()) {
                LOGD("play completed.");
                break;
            }
        } else {
            LOGE("read frame of packet failed.");
            if (callJavaHelper) {
                callJavaHelper->onError(THREAD_CHILD, FFMPEG_READ_PACKETS_FAIL);
            }
            break;
        }
    }

    isPlaying = 0;
    audioChannel->stop();
    videoChannel->stop();
}

void PlayerControl::setRenderFrameCallback(RenderFrameCallback renderCallback) {
    this->frameCallback = renderCallback;
}

int PlayerControl::get_duration() const {
    return duration;
}

void PlayerControl::seekTo(int point) {
    if (point < 0 || point > duration) {
        return;
    }
    if (!audioChannel && !videoChannel) {
        return;
    }
    if (!formatContext) {
        return;
    }
    pthread_mutex_lock(&seekMutex);
    /**
     * seek函数参数说明：
     * 1,上下文
     * 2，流索引，-1：表示选择的是默认流
     * 3，要seek到的时间戳
     * 4，seek的方式
     * AVSEEK_FLAG_BACKWARD： 表示seek到请求的时间戳之前的最靠近的一个关键帧
     * AVSEEK_FLAG_BYTE：基于字节位置seek
     * AVSEEK_FLAG_ANY：任意帧（可能不是关键帧，会花屏）
     * AVSEEK_FLAG_FRAME：基于帧数seek
     * */
    int ret = av_seek_frame(formatContext, -1, point * AV_TIME_BASE, AVSEEK_FLAG_BACKWARD);
    if (ret < 0) {
        if (callJavaHelper) {
            callJavaHelper->onError(THREAD_CHILD, ret);
        }
        return;
    }
    if (audioChannel) {
        audioChannel->pkt_queue.setWork(0);
        audioChannel->frame_queue.setWork(0);
        audioChannel->pkt_queue.clear();
        audioChannel->frame_queue.clear();
        //清除数据后，让队列重新工作
        audioChannel->pkt_queue.setWork(1);
        audioChannel->frame_queue.setWork(1);
    }
    if (videoChannel) {
        videoChannel->pkt_queue.setWork(0);
        videoChannel->frame_queue.setWork(0);
        videoChannel->pkt_queue.clear();
        videoChannel->frame_queue.clear();
        //清除数据后，让队列重新工作
        videoChannel->pkt_queue.setWork(1);
        videoChannel->frame_queue.setWork(1);
    }
    pthread_mutex_unlock(&seekMutex);
}

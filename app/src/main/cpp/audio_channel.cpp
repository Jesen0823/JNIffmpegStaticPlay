//
// Created by X1 Carbon on 2021/8/16.
//

#include "audio_channel.h"
#include "call_java_helper.h"

AudioChannel::AudioChannel(int id, CallJavaHelper *callJavaHelper,
                           AVCodecContext *codecContext, AVRational time_base)
        : BaseChannel(id, callJavaHelper, codecContext, time_base) {
    //缓冲区大小如何定？
    out_channels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
    out_sample_size = av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
    out_sample_rate = 44100;
    // 2(通道数) * 2（16bit=2字节）*44100（采样率）
    // 初始化buffer, out_pcm_buffers
    out_buffers_size = out_channels * out_sample_size * out_sample_rate;
    out_pcm_buffers = static_cast<uint8_t *>(malloc(out_buffers_size));
    memset(out_pcm_buffers, 0, out_buffers_size);
    swrContext = swr_alloc_set_opts(0, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16,
                                    out_sample_rate, codecContext->channel_layout,
                                    codecContext->sample_fmt, codecContext->sample_rate,
                                    0, 0);
    int ret = swr_init(swrContext);
    LOGE("swr_init ret is :%d", ret);
    if (ret != 0) {
        LOGE("swrContext init failed.");
    }
}

void *task_init_opensl(void *args) {
    AudioChannel *audioChannel = static_cast<AudioChannel *>(args);
    audioChannel->initOpenSL();
    return 0;
}

void *task_audio_decode(void *args) {
    AudioChannel *audioChannel = static_cast<AudioChannel *>(args);
    audioChannel->decodeAudio();
    return 0;
}

void AudioChannel::play() {
    /*swrContext = swr_alloc_set_opts(0, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16,
                                    out_sample_rate, codecContext->channel_layout,
                                    codecContext->sample_fmt, codecContext->sample_rate,
                                    0, 0);
    int ret = swr_init(swrContext);*/

    isPlaying = true;
    pkt_queue.setWork(1);
    frame_queue.setWork(1);
    // 创建初始化OPENSL_ES 的线程
    pthread_create(&pid_init_opensl, 0, task_init_opensl, this);
    pthread_create(&pid_audio_decode, 0, task_audio_decode, this);
}

void AudioChannel::stop() {

}

//4.3 创建回调函数
void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context) {
    AudioChannel *audioChannel = static_cast<AudioChannel *>(context);
    int pcm_size = audioChannel->getPCM();
    if (pcm_size > 0) {
        // 数据加入队列
        (*bq)->Enqueue(bq, audioChannel->out_pcm_buffers, pcm_size);
    }
}

void AudioChannel::initOpenSL() {
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

    /**
     * 1. 初始化引擎
     * */
    SLresult ret;
    ret = slCreateEngine(&engineObj, 0, NULL, 0, NULL, NULL);
    if (SL_RESULT_SUCCESS != ret) {
        LOGE("sl Create Engine failed");
        return;
    }
    ret = (*engineObj)->Realize(engineObj, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != ret) {
        LOGE("sl Realize failed");
        return;
    }
    // 1.3 获取引擎接口 SLEngineItf engineInterface相当于SurfaceHolder
    ret = (*engineObj)->GetInterface(engineObj, SL_IID_ENGINE, &engineInterface);
    if (SL_RESULT_SUCCESS != ret) {
        LOGE("sSLEngineI GetInterface failed");
        return;
    }
    /**
     * 2、设置混音器
     */
    // 2.1 创建混音器：SLObjectItf outputMixObject
    ret = (*engineInterface)->CreateOutputMix(engineInterface, &outputMixObj, 0,
                                              0, 0);
    if (SL_RESULT_SUCCESS != ret) {
        LOGE("CreateOutputMix failed");
        return;
    }
    // 2.2 初始化混音器
    ret = (*outputMixObj)->Realize(outputMixObj, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != ret) {
        LOGE("OutputMix Realize failed");
        return;
    }

    /**
     * 3、创建播放器
     */
    /**
     * pcm数据格式
     * SL_DATAFORMAT_PCM：数据格式为pcm格式
     * 2：双声道
     * SL_SAMPLINGRATE_44_1：采样率为44100
     * SL_PCMSAMPLEFORMAT_FIXED_16：采样格式为16bit
     * SL_PCMSAMPLEFORMAT_FIXED_16：数据大小为16bit
     * SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT：左右声道（双声道）
     * SL_BYTEORDER_LITTLEENDIAN：小端模式
     * */
    //3.1 配置输入声音信息
    //创建buffer缓冲类型的队列 2个队列
    SLDataLocator_AndroidSimpleBufferQueue bufferQueue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
                                                          2};
    SLDataFormat_PCM format_pcm = {SL_DATAFORMAT_PCM, 2,
                                   SL_SAMPLINGRATE_44_1,
                                   SL_PCMSAMPLEFORMAT_FIXED_16,
                                   SL_PCMSAMPLEFORMAT_FIXED_16,
                                   SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,
                                   SL_BYTEORDER_LITTLEENDIAN};

    //数据源 将上述配置信息放到这个数据源中
    SLDataSource audioSrc = {&bufferQueue, &format_pcm};

    //3.2 配置音轨（输出）
    //设置混音器
    SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX,
                                          outputMixObj};
    SLDataSink audioSnk = {&loc_outmix, NULL};
    //需要的接口 操作队列的接口
    const SLInterfaceID ids[1] = {SL_IID_BUFFERQUEUE};
    const SLboolean req[1] = {SL_BOOLEAN_TRUE};
    //3.3 创建播放器
    ret = (*engineInterface)->CreateAudioPlayer(engineInterface,
                                                &bqPlayerObj,  // 播放器
                                                &audioSrc,     // 播放器参数|缓冲队列|播放格式
                                                &audioSnk,     // 播放缓冲区
                                                1,             // 播放接口回调个数
                                                ids,           // 播放队列id
                                                req);          // 是否选择默认内置的播放队列
    if (SL_RESULT_SUCCESS != ret) {
        LOGE("CreateAudioPlayer failed");
        return;
    }
    //3.4 初始化播放器：SLObjectItf bqPlayerObject
    ret = (*bqPlayerObj)->Realize(bqPlayerObj, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != ret) {
        LOGE("init AudioPlayer failed");
        return;
    }
    //3.5 获取播放器接口：SLPlayItf bqPlayerPlay
    ret = (*bqPlayerObj)->GetInterface(bqPlayerObj, SL_IID_PLAY, &bqPlayerBufferQueue);
    if (SL_RESULT_SUCCESS != ret) {
        LOGE("AudioPlayer GetInterface failed");
        return;
    }
    /**
     * 4、设置播放回调函数
     */
    //4.1 获取播放器队列接口：SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue
    (*bqPlayerObj)->GetInterface(bqPlayerObj, SL_IID_BUFFERQUEUE, &bqPlayerBufferQueue);

    //4.2 设置回调 void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context)
    (*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue, bqPlayerCallback, this);

    /**
     * 5、设置播放器状态为可播放态
     */
    (*bqPlayerInterface)->SetPlayState(bqPlayerInterface, SL_PLAYSTATE_PLAYING);

    /**
     * 6、手动激活回调函数,会触发缓冲区不断地获取数据,主动去“要数据”
     */
    bqPlayerCallback(bqPlayerBufferQueue, this);
    LOGD("调用播放， packet size: %d", this->pkt_queue.size());
}

void AudioChannel::decodeAudio() {
    AVPacket *packet = 0; // 音频Packet
    while (isPlaying) {
        int ret = pkt_queue.pop(packet);
        if (!isPlaying) {
            break;
        }
        if (!ret) {
            continue;
        }
        // 拿到了数据包（编码压缩了的），需要把数据包给解码器进行解码
        ret = avcodec_send_packet(codecContext, packet);
        if (ret < 0) {
            LOGE("read audio packet failed.");
            break;
        }
        releaseAVPacket(&packet);

        AVFrame *frame = av_frame_alloc();
        ret = avcodec_receive_frame(codecContext, frame);
        if (ret == AVERROR(EAGAIN)) {
            continue; //重来
        } else if (ret != 0) {
            break;
        }
        while (isPlaying && frame_queue.size() > 100) {
            av_usleep(10 * 1000); // 10ms
            continue;
        }
        frame_queue.push(frame);// PCM数据
    }
    releaseAVPacket(&packet);
}

// 播放的时候才需要转换，即重采样
int AudioChannel::getPCM() {
    int pcm_data_size = 0;
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

        //假设输入10个数据，有可能这次转换只转换了8个，还剩2个数据（delay）
        //swr_get_delay: 下一个输入数据与下下个输入数据之间的时间间隔
        int64_t delay = swr_get_delay(swrContext, frame->sample_rate);

        //AV_ROUND_UP：向上取整
        int64_t out_nb_samples = av_rescale_rnd(frame->nb_samples + delay, frame->sample_rate,
                                                out_sample_rate,
                                                AV_ROUND_UP);
        /**
         * 转换
         * @param swrContext 上下文
         * @param out_pcm_buffers 输出缓冲区
         * @param out_nb_samples 输出缓冲区能容纳的最大数据量
         * @param frame->data 输入数据
         * @param nb_samples 输入数据量
         *
         * @return 转换后的sample个数
         * */
        int out_samples = swr_convert(swrContext, &out_pcm_buffers, out_nb_samples,
                                      (const uint8_t **) (frame->data), frame->nb_samples);

        // 获取swr_convert转换后 out_samples个 *2字节（16位）*2（双声道）
        pcm_data_size = out_samples * out_sample_size * out_channels;

        // 计算同步时钟：
        // (pts 是帧显示的开始时间) * time_base(特殊刻度单位) = 以time_base计算的帧开始显示时间
        syn_clock = frame->best_effort_timestamp * av_q2d(time_base);
        //syn_clock = frame->pts * av_q2d(time_base);
        LOGD("best_effort_timestamp VS pts: %\" PRId64\" VS %\" PRId64\" ...",
             frame->best_effort_timestamp, frame->pts);
        if (callJavaHelper) {
            callJavaHelper->onProgress(THREAD_CHILD, syn_clock);
        }
        break;
    }//end while
    releaseAVFrame(&frame);
    return pcm_data_size;
}


AudioChannel::~AudioChannel() {
    if (swrContext) {
        swr_free(&swrContext);
        swrContext = 0;
    }
    DELETE(out_pcm_buffers);
}
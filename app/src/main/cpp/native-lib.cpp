#include <jni.h>
#include <string>

#include <android/native_window_jni.h>
#include <android/log.h>
#include <unistd.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
// 音频相关
#include "libswresample/swresample.h"

}

/**
 * FFmpeg的上下文：
 * AVFormatContext: 用于获取视频流和音频流
 * AVCodecContext: 视频流解压上下文,可获取视频流信息
 *               可以得到解码器AVCodec(),解码器会解出HUV
 *               SwsContext: 转换上下文，用于YUV转RGB或其他可以显示的格式，最终绘制surfaceView显示
 *                          也可以旋转，全屏
 * ANativeWindow自带buffer,即底层是通过缓冲区来绘制的，缓冲区大小跟ANativeWindow一样大
 * 缓冲区拿到每一行首地址，把每一行RGB数据拷贝到缓冲区
 * */
#define LOGI(FORMAT,...) __android_log_print(ANDROID_LOG_INFO,"ffmpegPlay",FORMAT,##__VA_ARGS__);
#define LOGE(FORMAT,...) __android_log_print(ANDROID_LOG_ERROR,"ffmpegPlay",FORMAT,##__VA_ARGS__);


#define MAX_AUDIO_FRAME_SIZE 48000 * 4

extern "C" jstring Java_com_example_jniffmpegstaticplay_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {

    std::string hello = "Hello from C++\nffmpeg version:";
    return env->NewStringUTF(hello.c_str());
}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_jniffmpegstaticplay_StaticPlayer_native_1start(JNIEnv *env, jobject thiz,
                                                                jstring path_, jobject surface) {
    const char *path = env->GetStringUTFChars(path_, 0);
    __android_log_print(ANDROID_LOG_DEBUG,"TEST","path_ is:%s",path);

    // 初始化网络模块
    avformat_network_init();
    // 获取AVFormatContext
    AVFormatContext *formatContext = avformat_alloc_context();

    // 字典，配置信息
    AVDictionary *opts = NULL;
    // 第3个参数如果是0，会自动将2,3个参数key-value添加到第一个参数
    av_dict_set(&opts, "timeout", "3000000", 0);
    int ret = avformat_open_input(&formatContext, path, NULL, &opts);
    __android_log_print(ANDROID_LOG_DEBUG,"avformat_open_input","ret:%d",ret);

    if (ret) {
        return;
    }
    int video_stream_index = -1;
    avformat_find_stream_info(formatContext, NULL);
    for (int i = 0; i < formatContext->nb_streams; ++i) {
        if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;
            break;
        }
    }
    __android_log_print(ANDROID_LOG_DEBUG,"video_stream_index",":%d",video_stream_index);

    // 获取视频流解码参数
    AVCodecParameters *codecpar = formatContext->streams[video_stream_index]->codecpar;
    // 找到解码器
    AVCodec *dec = avcodec_find_decoder(codecpar->codec_id);
    // 解码器上下文
    AVCodecContext *codecContext = avcodec_alloc_context3(dec);
    // 加码器参数copy到解码上下文
    avcodec_parameters_to_context(codecContext, codecpar);
    // 打开解码器
    avcodec_open2(codecContext, dec, NULL);

    // 视频解码 H264-->YUV
    // 从数据流读取数据包，实例化AVPacket
    AVPacket *packet = av_packet_alloc();

    /** [获取转换上下文]
         * srcFormat:
         * desFormat:编码方式选择RGBA
         * flags:字节转换方式,以下几种：
           #重视速度 SWS_FAST_BILINEAR     1
           #重视速度 SWS_POINT          0x10
           #重视质量 SWS_BILINEAR          2
           #重视质量，锐度 SWS_BICUBIC           4
           #重视质量 SWS_GAUSS          0x80
           #重视质量，锐度 SWS_LANCZOS       0x200
           #重视质量，锐度 SWS_SPLINE        0x400
         * */
    SwsContext *swsContext = sws_getContext(codecContext->width, codecContext->height,
                                            codecContext->pix_fmt, codecContext->width,
                                            codecContext->height,
                                            AV_PIX_FMT_RGBA, SWS_BILINEAR, 0, 0, 0);

    ANativeWindow *nativeWindow = ANativeWindow_fromSurface(env, surface);
    ANativeWindow_Buffer aNativeWindowBuffer;

    ANativeWindow_setBuffersGeometry(nativeWindow, codecContext->width, codecContext->height,
                                     WINDOW_FORMAT_RGBA_8888);
    // 从数据流读取数据包，读取AVFrame队列
    while (av_read_frame(formatContext, packet) >= 0) {
        // 从数据流读取数据包，往packet发送数据帧
        avcodec_send_packet(codecContext, packet);
        AVFrame *frame = av_frame_alloc(); // 老版本用malloc()创建对象
        ret = avcodec_receive_frame(codecContext, frame);
        if (ret == AVERROR(EAGAIN)) {
            continue;
        } else if (ret < 0) {
            break;
        }
        // 绘制SurfaceView要用到RGB,用到SWsContext转换上下文
        //codecContext->pix_fmt

        // 接受转换后RGB数据的容器
        uint8_t *dst_data[0];
        // 每一行首地址,4代表R,G,B,A四个数组
        int dst_linesize[0];
        // 1代表左对齐
        av_image_alloc(dst_data, dst_linesize, codecContext->width, codecContext->height,
                       AV_PIX_FMT_RGBA, 1);

        if (packet->stream_index == video_stream_index) {
            if (ret == 0) {
                // 渲染
                ANativeWindow_lock(nativeWindow, &aNativeWindowBuffer, NULL);
                // frame->data就是一帧数据,frame->linesize是每一行首地址
                sws_scale(swsContext, reinterpret_cast<const uint8_t *const *>(frame->data),
                          frame->linesize, 0, frame->height, dst_data,
                          dst_linesize);
                int destStride = aNativeWindowBuffer.stride * 4;
                uint8_t *src_data = dst_data[0];
                int src_linesize = dst_linesize[0];
                uint8_t *firstWindow = static_cast<uint8_t *>(aNativeWindowBuffer.bits);

                for (int i = 0; i < aNativeWindowBuffer.height; ++i) {
                    // 内存拷贝，一行行渲染
                    memcpy(firstWindow + i * destStride, src_data + i * src_linesize, destStride);
                }
                ANativeWindow_unlockAndPost(nativeWindow);
                usleep(100 * 16);
                av_frame_free(&frame);
            }
        }
    }
    ANativeWindow_release(nativeWindow);
    avcodec_close(codecContext);
    avformat_free_context(formatContext);
    env->ReleaseStringUTFChars(path_, path);
}



extern "C"
JNIEXPORT void JNICALL
Java_com_example_jniffmpegstaticplay_AudioPlayer_soundPlay(JNIEnv *env, jobject thiz, jstring input_,
                                                           jstring output_) {
    const char *input = env->GetStringUTFChars(input_,0);
    const char *output = env->GetStringUTFChars(output_,0);
    avformat_network_init();

    // 总上下文
    AVFormatContext * formatContext = avformat_alloc_context();
    // 打开音频
    if (avformat_open_input(&formatContext, input, NULL, NULL) != 0){
        LOGI("%s", "无法打开音频文件");
        return;
    }
    //获取输入文件信息
    if(avformat_find_stream_info(formatContext,NULL) < 0){
        LOGI("%s","无法获取输入文件信息");
        return;
    }
    //视频时长（单位：微秒us，转换为秒需要除以1000000）
    int audio_stream_idx=-1;
    for (int i = 0; i < formatContext->nb_streams; ++i) {
        if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_idx=i;
            break;
        }
    }

    AVCodecParameters *codecpar = formatContext->streams[audio_stream_idx]->codecpar;
    //找到解码器
    AVCodec *dec = avcodec_find_decoder(codecpar->codec_id);
    //创建上下文
    AVCodecContext *codecContext = avcodec_alloc_context3(dec);
    avcodec_parameters_to_context(codecContext, codecpar);
    avcodec_open2(codecContext, dec, NULL);
    // 音频转换上下文
    SwrContext *swrContext = swr_alloc();

    // 输入的参数
    // 输入采样频率
    AVSampleFormat in_sample =  codecContext->sample_fmt;
    // 输入采样率
    int in_sample_rate = codecContext->sample_rate;
    // 输入声道布局
    uint64_t in_ch_layout=codecContext->channel_layout;

    // 输出参数是固定的
    // 输出采样格式
    AVSampleFormat out_sample=AV_SAMPLE_FMT_S16;
    // 输出采样
    int out_sample_rate=44100;
    // 输出声道布局
    uint64_t out_ch_layout = AV_CH_LAYOUT_STEREO;
    //设置转换器 的输入参数 和输出参数
    swr_alloc_set_opts(swrContext,out_ch_layout,out_sample,out_sample_rate
            ,in_ch_layout,in_sample,in_sample_rate,0,NULL);
    //初始化转换器其他的默认参数
    swr_init(swrContext);
    // 音频缓冲区
    uint8_t *out_buffer = (uint8_t *)(av_malloc(2 * 44100));
    FILE *fp_pcm = fopen(output, "wb");
    //读取包他是压缩数据
    AVPacket *packet = av_packet_alloc();
    int count = 0;
    // 设置音频缓冲区间 16bit   44100  PCM数据
    while (av_read_frame(formatContext, packet)>=0) {
        avcodec_send_packet(codecContext, packet);
        //解压缩数据，得到未压缩数据
        AVFrame *frame = av_frame_alloc();
        // 内部解压给frame
        int ret = avcodec_receive_frame(codecContext, frame);
        // frame是未压缩原始数据
        if (ret == AVERROR(EAGAIN))
            continue;
        else if (ret < 0) {
            LOGE("解码完成");
            break;
        }
        if (packet->stream_index!= audio_stream_idx) {
            continue;
        }
        LOGE("正在解码%d",count++);
        //frame  ---->统一的格式
        swr_convert(swrContext, &out_buffer, 2 * 44100,
                    (const uint8_t **)frame->data, frame->nb_samples);
        // 声道数
        int out_channerl_nb= av_get_channel_layout_nb_channels(out_ch_layout);
        //缓冲区的大小
        int out_buffer_size=  av_samples_get_buffer_size(NULL, out_channerl_nb, frame->nb_samples, out_sample, 1);
        fwrite(out_buffer,1,out_buffer_size, fp_pcm);
    }

    fclose(fp_pcm);
    av_free(out_buffer);
    swr_free(&swrContext);
    avcodec_close(codecContext);
    avformat_close_input(&formatContext);
    env->ReleaseStringUTFChars(input_, input);
    env->ReleaseStringUTFChars(output_, output);
}
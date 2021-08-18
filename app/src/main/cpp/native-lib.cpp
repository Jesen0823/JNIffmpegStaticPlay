// JNI层的总入口：
// 1. 播放由player_control.cpp管理
// 2. 音频由audio_channel.cpp管理，公共部分交由父类base_channel.h
// 3. 视频由video_channel.cpp管理，公共部分交由父类base_channel.h
//
// Created by X1 Carbon on 2021/8/16.
//

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

#include "player_control.h"
#include "call_java_helper.h"


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

CallJavaHelper *callJavaHelper;
ANativeWindow *window;
PlayerControl *playerControl;

JavaVM *javaVM = NULL;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;//静态初始化mutex

/**
 * 重点：获取到JavaVM
 * */
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    javaVM = vm;
    return JNI_VERSION_1_4;
}

/**
 * 渲染
 * linesize： yuv转换成的ARGB数据一行的像素个数
 * */
void renderCallback(uint8_t *data, int linesize, int width, int height) {
    LOGD("native_lib, renderCallback, linesize:%d, width:%d, height:%d", linesize, width, height);
    pthread_mutex_lock(&mutex);
    if (!window) {
        pthread_mutex_unlock(&mutex);
        return;
    }
    ANativeWindow_setBuffersGeometry(window, width, height, WINDOW_FORMAT_RGBA_8888);
    ANativeWindow_Buffer window_buffer;
    if (ANativeWindow_lock(window, &window_buffer, 0)) {
        ANativeWindow_release(window);
        window = 0;
        pthread_mutex_unlock(&mutex);
        return;
    }
    // windowBuffer是个有宽高的缓冲区，渲染是将数据内存一行行拷贝给windowBuffer
    uint8_t *dst_data = static_cast<uint8_t *>(window_buffer.bits); //缓冲区
    int window_linesize = window_buffer.stride * 4; // 一行像素
    uint8_t *src_data = data; // 数据源
    for (int i = 0; i < window_buffer.height; ++i) {
        // 内存拷贝
        memcpy(dst_data + i * window_linesize, src_data + i * linesize, window_linesize);
    }
    ANativeWindow_unlockAndPost(window);
    pthread_mutex_unlock(&mutex);
}

/****************************** 来自Java层的调用 *********************************/

extern "C"
JNIEXPORT void JNICALL
Java_com_example_jniffmpegstaticplay_JNIffPlayer_native_1set_1surface(JNIEnv *env, jobject thiz,
                                                                      jobject surface) {
    pthread_mutex_lock(&mutex);
    if (window) {
        ANativeWindow_release(window);
        window = 0;
    }
    // 创建窗口用于显示视频
    window = ANativeWindow_fromSurface(env, surface);
    pthread_mutex_unlock(&mutex);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_jniffmpegstaticplay_JNIffPlayer_native_1prepare(JNIEnv *env, jobject thiz,
                                                                 jstring path_) {
    const char *path = env->GetStringUTFChars(path_, 0);
    callJavaHelper = new CallJavaHelper(javaVM, env, thiz);

    playerControl = new PlayerControl(callJavaHelper, path);
    playerControl->setRenderFrameCallback(renderCallback);

    playerControl->prepare();

    env->ReleaseStringUTFChars(path_, path);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_jniffmpegstaticplay_JNIffPlayer_native_1start(JNIEnv *env, jobject thiz) {
    // 开始进入播放状态
    if (playerControl) {
        // 开始解码
        playerControl->start();
    }
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_example_jniffmpegstaticplay_JNIffPlayer_native_1get_1duration(JNIEnv *env, jobject thiz) {
    if (playerControl) {
        return (jint) playerControl->get_duration();
    }
    return 0;
}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_jniffmpegstaticplay_JNIffPlayer_native_1seek(JNIEnv *env, jobject thiz,
                                                              jint seek_point) {
    if (playerControl) {
        playerControl->seekTo(seek_point);
    }
}


extern "C"
JNIEXPORT void JNICALL
Java_com_example_jniffmpegstaticplay_JNIffPlayer_native_1pause(JNIEnv *env, jobject thiz) {
    if (playerControl) {
        playerControl->pause();
    }
}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_jniffmpegstaticplay_JNIffPlayer_native_1resume(JNIEnv *env, jobject thiz) {
    if (playerControl) {
        playerControl->resume();
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_jniffmpegstaticplay_JNIffPlayer_native_1stop(JNIEnv *env, jobject thiz) {
    if (playerControl) {
        playerControl->stop();
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_jniffmpegstaticplay_JNIffPlayer_native_1release(JNIEnv *env, jobject thiz) {
    pthread_mutex_lock(&mutex);
    if (window) {
        //把老的释放
        ANativeWindow_release(window);
        window = 0;
    }
    pthread_mutex_unlock(&mutex);
    DELETE(playerControl);
}

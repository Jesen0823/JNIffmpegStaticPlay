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

#define MAX_AUDIO_FRAME_SIZE 48000 * 4

CallJavaHelper *callJavaHelper;
ANativeWindow *window;
PlayerControl *playerControl;

// 重点：获取到JavaVM
JavaVM *javaVM = NULL;

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
    ANativeWindow_setBuffersGeometry(window, width, height, WINDOW_FORMAT_RGBA_8888);
    ANativeWindow_Buffer window_buffer;
    if (ANativeWindow_lock(window, &window_buffer, 0)) {
        ANativeWindow_release(window);
        window = 0;
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
JNIEXPORT void JNICALL
Java_com_example_jniffmpegstaticplay_JNIffPlayer_native_1set_1surface(JNIEnv *env, jobject thiz,
                                                                      jobject surface) {
    if (window) {
        ANativeWindow_release(window);
        window = 0;
    }
    // 创建窗口用于显示视频
    window = ANativeWindow_fromSurface(env, surface);
}
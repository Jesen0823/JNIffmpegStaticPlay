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
#define LOGI(FORMAT,...) __android_log_print(ANDROID_LOG_INFO,"ffmpegPlay",FORMAT,##__VA_ARGS__);
#define LOGE(FORMAT,...) __android_log_print(ANDROID_LOG_ERROR,"ffmpegPlay",FORMAT,##__VA_ARGS__);

#define MAX_AUDIO_FRAME_SIZE 48000 * 4

CallJavaHelper *callJavaHelper;
ANativeWindow *window;
PlayerControl *playerControl;

// 重点：获取到JavaVM
JavaVM *javaVM = NULL;
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved){
    javaVM = vm;
    return JNI_VERSION_1_4;
}


extern "C"
JNIEXPORT void JNICALL
Java_com_example_jniffmpegstaticplay_JNIffPlayer_native_1prepare(JNIEnv *env, jobject thiz,
                                                                 jstring path_) {
    const char *path = env->GetStringUTFChars(path_,0);
    callJavaHelper = new CallJavaHelper(javaVM, env, thiz);

    playerControl = new PlayerControl(callJavaHelper, path);
    playerControl->prepare();

    env->ReleaseStringUTFChars(path_,path);
}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_jniffmpegstaticplay_JNIffPlayer_native_1start(JNIEnv *env, jobject thiz) {
    // TODO: implement native_start()
}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_jniffmpegstaticplay_JNIffPlayer_native_1set_1surface(JNIEnv *env, jobject thiz,
                                                                      jobject surface) {
    if (window){
        ANativeWindow_release(window);
        window = 0;
    }
    // 创建窗口用于显示视频
    window = ANativeWindow_fromSurface(env, surface);

}
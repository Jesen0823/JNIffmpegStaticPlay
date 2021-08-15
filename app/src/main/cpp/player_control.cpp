//
// Created by X1 Carbon on 2021/8/15.
//

#include "player_control.h"
#include "macro.h"

#define LOGI(FORMAT,...) __android_log_print(ANDROID_LOG_INFO,"ffmpegPlay",FORMAT,##__VA_ARGS__);
#define LOGE(FORMAT,...) __android_log_print(ANDROID_LOG_ERROR,"ffmpegPlay",FORMAT,##__VA_ARGS__);


void *runPrepare(void *args){
    PlayerControl *playerControl =static_cast<PlayerControl *> (args);
    //playerControl->prepare();
    playerControl->prepareControl();
    // 一定要返回0
    return 0;
}

PlayerControl::PlayerControl(CallJavaHelper *callJavaHelper, const char *path) {
    this->callJavaHelper = callJavaHelper;
    url = new char [strlen(path) +1];
    strcpy(url, path);
}

PlayerControl::~PlayerControl() {

}

void PlayerControl::prepare() {
    // 传递this对象为了run函数中playerControl可以调用成员方法prepare()
    pthread_create(&pid_prepare,NULL,runPrepare, this);
}

// 开始视频渲染,该方法中子线程可以访问对象的属性
void PlayerControl::prepareControl() {
    // 初始化网络模块
    avformat_network_init();
    formatContext = avformat_alloc_context();
    //1、打开URL
    AVDictionary *opts = NULL;
    //设置超时3秒
    av_dict_set(&opts, "timeout", "3000000", 0);
    //强制指定AVFormatContext中AVInputFormat的。这个参数一般情况下可以设置为NULL，这样FFmpeg可以自动检测AVInputFormat。
    //输入文件的封装格式
    //av_find_input_format("avi") // 可以是rtmp
    // ret为0 表示成功
    int ret = avformat_open_input(&formatContext, url, NULL, &opts);
    if (ret!= 0){
        LOGE("prepareControl"," open failed.");
        callJavaHelper->onError(THREAD_CHILD, FFMPEG_CAN_NOT_OPEN_URL);
    }
    avformat_find_stream_info(formatContext, NULL);
}

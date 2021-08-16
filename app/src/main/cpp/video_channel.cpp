//
// Created by X1 Carbon on 2021/8/16.
//

#include "video_channel.h"

VideoChannel::VideoChannel(int id, CallJavaHelper *callJavaHelper, AVCodecContext *codecContext)
: BaseChannel(id, avCodecContext,callJavaHelper){

}

VideoChannel::~VideoChannel() {

}

void VideoChannel::play() {
    pkt_queue.setWork(1);
    frame_queue.setWork(1);
}

void VideoChannel::stop() {

}

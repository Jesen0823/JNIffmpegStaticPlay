//
// Created by X1 Carbon on 2021/8/16.
//

#include "audio_channel.h"
#include "call_java_helper.h"
#include "player_control.h"

AudioChannel::AudioChannel(int id, CallJavaHelper *callJavaHelper,
                           AVCodecContext *codecContext):BaseChannel(id, avCodecContext, callJavaHelper) {

}

AudioChannel::~AudioChannel() {

}

void AudioChannel::play() {

}

void AudioChannel::stop() {

}

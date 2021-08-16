//
// Created by X1 Carbon on 2021/8/16.
//

#include "base_channel.h"
#include "player_control.h"

BaseChannel::BaseChannel(volatile int channelId, AVCodecContext *avCodecContext,
                         CallJavaHelper *callJavaHelper) : channelId(channelId),
                                                           avCodecContext(avCodecContext),
                                                           callJavaHelper(callJavaHelper) {}

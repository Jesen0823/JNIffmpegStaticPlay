//
// Created by X1 Carbon on 2021/8/16.
//

#ifndef JNIFFMPEGSTATICPLAY_VIDEO_CHANNEL_H
#define JNIFFMPEGSTATICPLAY_VIDEO_CHANNEL_H

class VideoChannel{

public:
    VideoChannel(int id, CallJavaHelper *callJavaHelper, AVCodecContext *codecContext);

    ~VideoChannel();
};

#endif //JNIFFMPEGSTATICPLAY_VIDEO_CHANNEL_H

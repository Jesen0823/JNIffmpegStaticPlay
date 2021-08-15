//
// Created by X1 Carbon on 2021/8/15.
// c调用java的反射工具类

#include <jni.h>
#include "macro.h"

#ifndef JNIFFMPEGSTATICPLAY_CALL_JAVA_HELPER_H
#define JNIFFMPEGSTATICPLAY_CALL_JAVA_HELPER_H

class CallJavaHelper{
public:
    CallJavaHelper(JavaVM *_javaVM, JNIEnv *_env, jobject &_jobj);

    ~CallJavaHelper();

    void onParpare(int thread);

    void onProgress(int thread, int progress);

    void onError(int thread, int code);

private:
    JavaVM *javaVm;
    JNIEnv *env;
    jobject jobj;
    jmethodID jmethod_id_prepare;
    jmethodID jmethod_id_progress;
    jmethodID jmethod_id_error;
};

#endif //JNIFFMPEGSTATICPLAY_CALL_JAVA_HELPER_H

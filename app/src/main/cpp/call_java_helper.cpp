//
// Created by X1 Carbon on 2021/8/15.
//

#include "call_java_helper.h"

// 初始化类的参数，方法1：
/*CallJavaHelper::CallJavaHelper(JavaVM *_javaVM, JNIEnv *_env, jobject &_jobj) {
    this->javaVm = _javaVM;
    this->env = _env;
    this->jobj = _jobj;
}*/

// 初始化类的参数，方法2：
CallJavaHelper::CallJavaHelper(JavaVM *_javaVM, JNIEnv *_env, jobject &_jobj)
: javaVm(_javaVM), env(_env){
    jobj = env->NewGlobalRef(_jobj);

    jclass jclazz = env->GetObjectClass(jobj);
    // 得到方法引用 ArtMethod
    jmethod_id_prepare = env->GetMethodID(jclazz,"onPrepare","()");
    jmethod_id_progress = env->GetMethodID(jclazz,"onProgress","(I)V");
    jmethod_id_error = env->GetMethodID(jclazz,"onError","(I)V");

}

CallJavaHelper::~CallJavaHelper() {

}

// 调用java层： JNIffPlayer.onPrepare()
void CallJavaHelper::onParpare(int thread) {

}

void CallJavaHelper::onProgress(int thread, int progress) {

}

void CallJavaHelper::onError(int thread, int code) {
    if (thread == THREAD_CHILD){
       // 子线程的话需要绑定JavaVM
       JNIEnv *jniEnv;
       // 如果已经绑定
        if (javaVm->AttachCurrentThread(&jniEnv,0) != JNI_OK){
            return;
        }
        jniEnv->CallVoidMethod(jobj, jmethod_id_error, code);
        // 调用完解绑javaJVM
        javaVm->DetachCurrentThread();
    } else{
        // 主线程
        env->CallVoidMethod(jobj, jmethod_id_error, code);
    }
}

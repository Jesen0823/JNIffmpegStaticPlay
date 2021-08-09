#include <jni.h>
#include <string>

/*extern{
#include <libavcodec/avcodec.h>
}*/


extern "C" jstring Java_com_example_jniffmpegstaticplay_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}
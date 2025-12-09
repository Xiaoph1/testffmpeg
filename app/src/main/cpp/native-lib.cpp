#include <jni.h>
#include <string>
#include<android/log.h>
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,"testff",__VA_ARGS__)
extern "C"{
#include <libavcodec/avcodec.h>
#include <libavformat//avformat.h>
}

extern "C" JNIEXPORT jstring
JNICALL
Java_com_example_testffmpeg_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    hello += avcodec_configuration();

    //舒适化解封装
    av_register_all();
    //初始化网络
    avformat_network_init();
    //打开文件
    AVFormatContext *ic = NULL;
    char path[] = "/sdcard/1.mp4";
    int re = avformat_open_input(&ic,path,nullptr,nullptr);
    if (re == 0){
        //打开成功
        LOGW("avformat_open_input %s success!",path);
        avformat_close_input(&ic);
    }else{
        LOGW("avformat_open_input %s failed:",av_err2str(re));
    }
    return env->NewStringUTF(hello.c_str());
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_testffmpeg_MainActivity_open(JNIEnv *env, jobject thiz, jstring url,
                                              jobject handle) {
    const char *url_str = env->GetStringUTFChars(url, nullptr);
    FILE *fp = fopen(url_str, "rb");
    if (!fp){
        LOGW("%s open failed!",url_str);
    }else{
        LOGW("%s open success!",url_str);
        fclose(fp);
    }

}
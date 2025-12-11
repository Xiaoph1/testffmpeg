#include <jni.h>
#include <string>
#include<android/log.h>
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,"testff",__VA_ARGS__)
extern "C"{
#include <libavcodec/avcodec.h>
#include <libavformat//avformat.h>
}
#include<iostream>
using namespace std;

static double r2d(AVRational r){
    return r.num == 0 || r.den == 0?0:(double)r.num/(double)r.den;
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
    if (re != 0){
        //打开失败
        LOGW("avformat_open_input %s failed:",av_err2str(re));
        return env->NewStringUTF(hello.c_str());

    }
    //打开成功
    LOGW("avformat_open_input %s success!",path);
    //获取流信息，在没有成功获取ic->duration,ic->nb_streams时使用
    //re = avformat_find_stream_info(ic,0);
    //然后获取信息
    LOGW("duration = %lld, nb_streams = %d",ic->duration,ic->nb_streams);

    //方法一：遍历获取音视频流信息
    int fps=0,videoStream=0,audioStream=0;
    for (int i = 0; i < ic->nb_streams; i++) {
        AVStream *as = ic->streams[i];
        if (as->codecpar->codec_type == AVMEDIA_TYPE_VIDEO){
            LOGW("视频数据");
            videoStream = i;
            fps = r2d(as->avg_frame_rate);
            LOGW("fps = %d, width = %d,height = %d,codeid = %d,pixformat = %d",fps,
                 as->codecpar->width,
                 as->codecpar->height,
                 as->codecpar->codec_id,
                 as->codecpar->format);
        }
        else if (as->codecpar->codec_type == AVMEDIA_TYPE_AUDIO){
            LOGW("音频数据");
            audioStream = i;
            LOGW("sample_rate = %d, channels = %d,sample_format = %d",
                 as->codecpar->sample_rate,
                 as->codecpar->channels,
                 as->codecpar->format);
        }

    }
    //方法二：av_find_best_stream找到索引
    audioStream = av_find_best_stream(ic,AVMEDIA_TYPE_AUDIO,-1,-1,NULL,0);
    LOGW("av_find_best_stream:%d",audioStream);

    //读取帧数据
    AVPacket *pkt = av_packet_alloc();
    for (;;) {
        int re = av_read_frame(ic,pkt);
        if (re != 0){
            LOGW("av_read_frame to the end");
            int pos = 10 * r2d(ic->streams[videoStream]->time_base);
            av_seek_frame(ic,videoStream,pos,AVSEEK_FLAG_BACKWARD|AVSEEK_FLAG_FRAME);
        }
        LOGW("stream = %d size=%d pts=%lld flag=%d",
             pkt->stream_index,pkt->size,pkt->pts,pkt->flags);

        //释放空间
        av_packet_unref(pkt);
    }


    avformat_close_input(&ic);
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
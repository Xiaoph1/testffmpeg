#include <jni.h>
#include <string>
#include<android/log.h>
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,"testff",__VA_ARGS__)
extern "C"{
#include <libavcodec/avcodec.h>
#include <libavformat//avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}
#include<iostream>
using namespace std;

static double r2d(AVRational r){
    return r.num == 0 || r.den == 0?0:(double)r.num/(double)r.den;
}
long long GetNowMs(){
    struct timeval tv;
    gettimeofday(&tv,NULL);
    int sec = tv.tv_sec%360000;
    long long t = sec*1000 + tv.tv_usec/1000;
    return t;
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
    //初始化解码器
    avcodec_register_all();
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

    //打开视频解码器
    //软解码器
    AVCodec *codec = avcodec_find_decoder(ic->streams[videoStream]->codecpar->codec_id);
    //硬解码
//    codec = avcodec_find_decoder_by_name("h264_mediacodec");
    if (!codec){
        LOGW("avcodec_find failed");
        return env->NewStringUTF(hello.c_str());
    }
    //解码器初始化
    AVCodecContext *vc = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(vc,ic->streams[videoStream]->codecpar);
    vc->thread_count = 8;
    //打开解码器
    re = avcodec_open2(vc,0,0);
    if (re != 0){
        LOGW("avcodec_open2 video failed");
        return env->NewStringUTF(hello.c_str());
    }
    LOGW("avcodec_open2 video success");

    //打开音频解码器
    //软解码器
    AVCodec *acodec = avcodec_find_decoder(ic->streams[audioStream]->codecpar->codec_id);
    //硬解码
//    acodec = avcodec_find_decoder_by_name("h264_mediacodec");
    if (!acodec){
        LOGW("avcodec_find audio failed");
        return env->NewStringUTF(hello.c_str());
    }
    //解码器初始化
    AVCodecContext *ac = avcodec_alloc_context3(acodec);
    avcodec_parameters_to_context(ac,ic->streams[audioStream]->codecpar);
    ac->thread_count = 1;
    //打开解码器
    re = avcodec_open2(ac,0,0);
    if (re != 0){
        LOGW("avcodec_open2 audio failed");
        return env->NewStringUTF(hello.c_str());
    }
    LOGW("avcodec_open2 audio success");


    //读取帧数据
    AVPacket *pkt = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();

    //初始化像素格式转换上下文
    SwsContext *vctx = NULL;
    int outWidth = 1280;
    int outHeight = 720;

    //time
    long long start = GetNowMs();
    int frame_count = 0;
    char*rgb = new char[1920*1080*4];
    for (;;) {
        //计算三秒内视频解码多少帧/秒
        if (GetNowMs() -start >= 3000){
            LOGW("now decode fps is %d",frame_count/3);
            start = GetNowMs();
            frame_count = 0;
        }
        int re = av_read_frame(ic,pkt);
        if (re != 0){
            LOGW("av_read_frame to the end");
            int pos = 10 * r2d(ic->streams[videoStream]->time_base);
            av_seek_frame(ic,videoStream,pos,AVSEEK_FLAG_BACKWARD|AVSEEK_FLAG_FRAME);
//            pkt = nullptr;
        }

        //音视频同步解码
        AVCodecContext *cc = vc;
        if (pkt->stream_index != videoStream){
            cc = ac;
        }

        LOGW("stream = %d size=%d pts=%lld flag=%d",
             pkt->stream_index,pkt->size,pkt->pts,pkt->flags);
        //解码
        //发送到线程中解码
        re = avcodec_send_packet(cc,pkt);
        //释放空间
        av_packet_unref(pkt);

        if (re != 0){
            LOGW("avcodec_send_packet %d failed",pkt->stream_index);
            continue;
        }
        for (;;) {
            re = avcodec_receive_frame(cc,frame);
            if (re != 0){
//                LOGW("avcodec_receive_frame failed");
                break;
            }
            //视频帧++
            if (cc == vc){
                frame_count++;
                vctx = sws_getCachedContext(vctx,
                                            frame->width,
                                            frame->height,
                                            (AVPixelFormat)frame->format,
                                            outWidth,
                                            outHeight,
                                            AV_PIX_FMT_RGBA,
                                            SWS_FAST_BILINEAR,
                                            0,0,0);
                if (!vctx){
                    LOGW("sws_getCachedContext failed");
                }else{
                    uint8_t *data[AV_NUM_DATA_POINTERS] = {0};
                    data[0] =(uint8_t *)rgb;
                    int lines[AV_NUM_DATA_POINTERS] = {0};
                    lines[0] = outWidth * 4;
                    int h = sws_scale(vctx,frame->data,frame->linesize,0,frame->height,
                                      data,lines);
                    LOGW("sws_scale = %d",h);
                }
            }


            LOGW("avcodec_receive_frame success,%lld",frame->pts);
        }

    }

    delete[](rgb);
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
#include <jni.h>
#include <string>
#include<android/log.h>
#include<android/native_window.h>
#include<android/native_window_jni.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,"testff",__VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_ERROR,"test",##__VA_ARGS__);
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
static SLObjectItf engineSL = NULL;
SLEngineItf CreateSL() {
    SLresult re;
    SLEngineItf en;
    re = slCreateEngine(&engineSL,0,0,0,0,0);
    if (re != SL_RESULT_SUCCESS) return NULL;
    re = (*engineSL)->Realize(engineSL,SL_BOOLEAN_FALSE);
    if (re != SL_RESULT_SUCCESS) return NULL;
    re = (*engineSL)->GetInterface(engineSL,SL_IID_ENGINE,&en);
    if (re != SL_RESULT_SUCCESS) return NULL;
    return en;
}
void PcmCall(SLAndroidSimpleBufferQueueItf bf,void *contex){
    LOGD("pcm call");
    static FILE  *fp = NULL;
    static char *buf = NULL;
    if (!buf){
        buf = new char[1024*1024];
    }
    if (!fp){
       fp = fopen("/sdcard/test.pcm","rb");
    }
    if (!fp) return;
    //没到结尾
    if (feof(fp) == 0){
        int len = fread(buf,1,1024,fp);
        if (len>0){
            (*bf)->Enqueue(bf,buf,len);
        }
    }




}
extern "C" JNIEXPORT jstring
JNICALL
Java_com_example_testffmpeg_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Hello from C++";

    //1.创建引擎
    SLEngineItf eng = CreateSL();
    if (eng){
        LOGD("CreateSL success");
    }else{
        LOGD("CreateSL failed");
    }

    //2.创建混音器
    SLresult re;
    SLObjectItf mix = NULL;
    re = (*eng)->CreateOutputMix(eng,&mix,0,nullptr,nullptr);
    if (re != SL_RESULT_SUCCESS) {
        LOGD("CreateOutputMix failed");
    }
    re = (*mix)->Realize(mix,SL_BOOLEAN_FALSE);
    if (re != SL_RESULT_SUCCESS) {
        LOGD("Realize Mix failed");
    }
    SLDataLocator_OutputMix outmix = {SL_DATALOCATOR_OUTPUTMIX,mix};
    SLDataSink audioSink = {&outmix,0};

    //3.配置音频信息
    SLDataLocator_AndroidSimpleBufferQueue que = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,10};
    SLDataFormat_PCM pcm = {
            SL_DATAFORMAT_PCM,
            2,
            SL_SAMPLINGRATE_44_1,
            SL_PCMSAMPLEFORMAT_FIXED_16,
            SL_PCMSAMPLEFORMAT_FIXED_16,
            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,
            SL_BYTEORDER_LITTLEENDIAN
    };
    SLDataSource ds = {&que,&pcm};

    //4.创建播放器
    SLObjectItf player = NULL;
    SLPlayItf iplayer = NULL;
    SLAndroidSimpleBufferQueueItf pcmQue = NULL;
    const SLInterfaceID ids[] = {SL_IID_BUFFERQUEUE};
    const SLboolean req[] = {SL_BOOLEAN_TRUE};

    re = (*eng)->CreateAudioPlayer(eng,&player,&ds,&audioSink,sizeof (ids)/sizeof (SLInterfaceID),ids,req);
    if (re != SL_RESULT_SUCCESS) {
        LOGD("CreateAudioPlayer failed");
    }else{
        LOGD("CreateAudioPlayer success");
    }
    (*player)->Realize(player,SL_BOOLEAN_FALSE);
    //获取player接口
    re = (*player)->GetInterface(player,SL_IID_PLAY,&iplayer);
    if (re != SL_RESULT_SUCCESS) {
        LOGD("GetInterface SL_IID_PLAY failed");
    }else{
        LOGD("GetInterface SL_IID_PLAY success");
    }
    //获取队列接口
    re = (*player)->GetInterface(player,SL_IID_BUFFERQUEUE,&pcmQue);
    if (re != SL_RESULT_SUCCESS) {
        LOGD("GetInterface SL_IID_BUFFERQUEUE failed");
    }else{
        LOGD("GetInterface SL_IID_BUFFERQUEUE success");
    }
    //设置回调函数，播放队列空调用
    (*pcmQue)->RegisterCallback(pcmQue,PcmCall,0);
    //设置播放状态
    (*iplayer)->SetPlayState(iplayer,SL_PLAYSTATE_PLAYING);
    //启动队列回调
    (*pcmQue)->Enqueue(pcmQue,"",1);

    return env->NewStringUTF(hello.c_str());
}


extern "C"
JNIEXPORT void JNICALL
Java_com_example_testffmpeg_MainActivity_open(JNIEnv *env, jobject thiz, jstring url,
                                                   jobject surface) {
    if (surface== nullptr){
        LOGW("here43");
        return;
    }
    const char *path = env->GetStringUTFChars(url,0);

    //舒适化解封装
    av_register_all();
    //初始化网络
    avformat_network_init();
    //初始化解码器
    avcodec_register_all();
    //打开文件
    AVFormatContext *ic = NULL;
    int re = avformat_open_input(&ic,path,nullptr,nullptr);
    if (re != 0){
        //打开失败
        LOGW("avformat_open_input %s failed:",av_err2str(re));
        return ;
    }
    //打开成功
    LOGW("avformat_open_input %s success!",path);
    //获取流信息，在没有成功获取ic->duration,ic->nb_streams时使用
    re = avformat_find_stream_info(ic,0);
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
        return ;
    }
    //解码器初始化
    AVCodecContext *vc = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(vc,ic->streams[videoStream]->codecpar);
    vc->thread_count = 8;
    //打开解码器
    re = avcodec_open2(vc,0,0);
    if (re != 0){
        LOGW("avcodec_open2 video failed");
        return;
    }
    LOGW("avcodec_open2 video success");

    //打开音频解码器
    //软解码器
    AVCodec *acodec = avcodec_find_decoder(ic->streams[audioStream]->codecpar->codec_id);
    //硬解码
//    acodec = avcodec_find_decoder_by_name("h264_mediacodec");
    if (!acodec){
        LOGW("avcodec_find audio failed");
        return ;
    }
    //解码器初始化
    AVCodecContext *ac = avcodec_alloc_context3(acodec);
    avcodec_parameters_to_context(ac,ic->streams[audioStream]->codecpar);
    ac->thread_count = 1;
    //打开解码器
    re = avcodec_open2(ac,0,0);
    if (re != 0){
        LOGW("avcodec_open2 audio failed");
        return ;
    }
    LOGW("avcodec_open2 audio success");


    //读取帧数据
    AVPacket *pkt = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();

    //初始化像素格式转换上下文
    SwsContext *vctx = NULL;
    int outWidth = 1280;
    int outHeight = 720;
    char*rgb = new char[outWidth * outHeight * 4];
    char *pcm = new char[48000*4*2];

    //time
    long long start = GetNowMs();
    int frame_count = 0;

    //音频重采样
    SwrContext *actx = swr_alloc();
    actx = swr_alloc_set_opts(actx,
                              av_get_default_channel_layout(2),
                              AV_SAMPLE_FMT_S16,
                              ac->sample_rate,
                              av_get_default_channel_layout(ac->channels),
                              ac->sample_fmt,
                              ac->sample_rate,
                              0,0);
    re = swr_init(actx);
    if (re != 0){
        LOGW("swr_init failed");
    }else{
        LOGW("swr_init success");
    }

    //显示窗口初始化
    ANativeWindow *nwin = ANativeWindow_fromSurface(env,surface);
    ANativeWindow_setBuffersGeometry(nwin,outWidth,outHeight,WINDOW_FORMAT_RGBA_8888);
    ANativeWindow_Buffer wbuf;

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
//            int pos = 10 * r2d(ic->streams[videoStream]->time_base);
//            av_seek_frame(ic,videoStream,pos,AVSEEK_FLAG_BACKWARD|AVSEEK_FLAG_FRAME);
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
                    if (h>0){
                        ANativeWindow_lock(nwin,&wbuf,0);
                        uint8_t *dst = (uint8_t*)wbuf.bits;
                        memcpy(dst,rgb,outWidth*outHeight*4);
                        ANativeWindow_unlockAndPost(nwin);

                    }
                }
            }
                //音频帧
            else{
                uint8_t *out[2] = {0};
                out[0] = (uint8_t*)pcm;
                //音频重采样
                int len = swr_convert(actx,out,
                                      frame->nb_samples,
                                      (const uint8_t**)frame->data,
                                      frame->nb_samples);
                LOGW("swr_convert = %d",len);
            }

            LOGW("avcodec_receive_frame success,%lld",frame->pts);
        }

    }

    delete[] rgb;
    delete[] pcm;
    avformat_close_input(&ic);
    env->ReleaseStringUTFChars(url,path);
}
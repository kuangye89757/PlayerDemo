#include <jni.h>
#include <string>
#include <unistd.h>
#include <android/native_window_jni.h>
#include <zconf.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavformat/avformat.h>
}


extern "C"
JNIEXPORT void JNICALL
Java_com_diaochan_playerdemo_WangyiPlayer_native_1start(JNIEnv *env, jobject thiz, jstring path_,
                                                        jobject surface) {

    const char *path = env->GetStringUTFChars(path_, 0);

   
    /**
     * FFmpeg 视频绘制开始
     */
    avformat_network_init();//初始化网络模块

    /**
     * 第一步：获取AVFormatContext (针对当前视频的 总上下文)
     */
    AVFormatContext *formatContext = avformat_alloc_context();

    // 设置视频相关信息
    AVDictionary *opts = NULL;
    av_dict_set(&opts, "timeout", "3000000", 0);//3秒超时 单位微秒

    // 打开视频文件
    int ret = avformat_open_input(&formatContext, path, NULL, &opts);
    if (ret) {
        //在 FFmpeg的函数中一般，返回0表示成功
        return;
    }
    avformat_find_stream_info(formatContext, NULL);//通知FFmpeg解析流

    // 遍历上下文中的所有流，找到视频流
    int video_stream_index = -1;
    for (int i = 0; i < formatContext->nb_streams; ++i) {
        /**
         * AVMEDIA_TYPE_VIDEO -- 视频
         * AVMEDIA_TYPE_AUDIO -- 音频
         * AVMEDIA_TYPE_SUBTITLE -- 字幕
         */
        if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;
            break;
        }
    }

    // 获取视频流解码参数 AVCodecParameters
    AVCodecParameters *codecpar = formatContext->streams[video_stream_index]->codecpar;

    /**
     * 第二步：根据AVCodecParameters中的id,获取解码器AVCodec  
     * 并创建解码器上下文 AVCodecContext
     * 
     * 【带数字的函数，越大数字表示越新】
     */
    AVCodec *avcodec = avcodec_find_decoder(codecpar->codec_id);
    AVCodecContext *codecContext = avcodec_alloc_context3(avcodec);

    //将解码器参数传入到上下文
    // 这样解码器上下文AVCodecContext 和 解码器AVCodec 都有参数的信息
    avcodec_parameters_to_context(codecContext, codecpar);

    /**
     * 第三步：打开解码器
     */
    avcodec_open2(codecContext, avcodec, NULL);


    /**
     * 第四步：解码 YUV数据（存在于AVPacket）
     * 
     */
    AVPacket *packet = av_packet_alloc();


    /**
     *  转换上下文SwsContext初始化
     *  
     * （通过转换上下文SwsContext 转换帧数据为RGB图像数据）
     * 
     * codecContext->width,codecContext->height 视频的宽高可通过播放器信息查看
     * codecContext->pix_fmts 视频编码方式
     * 
     * 转换方式:
     *      重视速度：fast_bilinear,point
     *      重视质量：cubic,spline,lanczos,gauss
     *      
     * 
     * 
     * 
     * struct SwsContext *sws_getContext(
     * 
     *    int srcW, int srcH, enum AVPixelFormat srcFormat, //输入源
          int dstW, int dstH, enum AVPixelFormat dstFormat, //输出源
          int flags, //转换方式
          SwsFilter *srcFilter,SwsFilter *dstFilter, const double *param);
     */
    SwsContext *swsContext = sws_getContext(codecContext->width, codecContext->height,
                                            codecContext->pix_fmt,
                                            codecContext->width, codecContext->height,
                                            AV_PIX_FMT_RGBA,
                                            SWS_BILINEAR, 0, 0, 0);

    /**
    * 创建一个来自于Surface的Window
    * 内部有一个缓冲区 用来底层渲染
    */
    ANativeWindow *nativeWindow = ANativeWindow_fromSurface(env, surface);
    //设置缓冲区的大小
    ANativeWindow_setBuffersGeometry(nativeWindow, codecContext->width, codecContext->height,
                                     WINDOW_FORMAT_RGBA_8888);
    //视频缓冲区
    ANativeWindow_Buffer outBuffer;

    //从视频流中读取一个数据包 返回值小于0表示读取完成或错误
    while (av_read_frame(formatContext, packet) >= 0) {
        avcodec_send_packet(codecContext, packet);
        AVFrame *frame = av_frame_alloc();
        ret = avcodec_receive_frame(codecContext, frame);
        if (ret == AVERROR(EAGAIN)) {
            //重试
            continue;
        } else if (ret < 0) {
            //结束位
            break;
        }

        /**
         * 第五步：绘制 （通过转换上下文SwsContext AVFrame转换为RGB图像数据）
         */
        uint8_t *dst_data[0];//接收的容器 rgba
        int dst_linesize[0];//每一行首地址
        //开辟空间 赋值dst_linesize
        av_image_alloc(dst_data, dst_linesize, codecContext->width, codecContext->height,
                       AV_PIX_FMT_RGBA, 1/**左对齐*/);


        
        if(packet->stream_index == video_stream_index && ret == 0){


            /**
             * 第六步：渲染到surface
             * 
             * AVFrame(yuv) --> Image(RGBA dst_data) --> surfaceView
             */
            ANativeWindow_lock(nativeWindow, &outBuffer, NULL);//上锁
            
            //帧的首地址:frame->linesize
            /**输入源信息*/
            // 参数1：转换上下文
            // 参数2：当前帧数据
            // 参数3：当前帧首地址 （知道首地址就能绘制整个帧）
            // 参数4：偏移量 （这里不用偏移）
            /**输出源信息*/
            // 参数5：接收的容器 rgba
            sws_scale(swsContext, reinterpret_cast<const uint8_t *const *>(frame->data),
                      frame->linesize, 0, frame->height,
                      dst_data, dst_linesize);

            //输入源
            int destStride = outBuffer.stride * 4;//获取一行多少字节
            uint8_t *src_data = dst_data[0];
            int src_linesize = dst_linesize[0];
            uint8_t *firstWindown = static_cast<uint8_t *>(outBuffer.bits);
            
            
            for (int i = 0; i < outBuffer.height; ++i) {
                //内存拷贝 加快渲染效率
                memcpy(firstWindown + i * destStride, src_data + i * src_linesize, destStride);
            }

            ANativeWindow_unlockAndPost(nativeWindow);//解锁
            usleep(1000 * 16);
            av_frame_free(&frame);
        }
    }

    /**
     * 第七步：资源释放
     */
    ANativeWindow_release(nativeWindow);
    avcodec_close(codecContext);
    avformat_free_context(formatContext);
    env->ReleaseStringUTFChars(path_, path);
}
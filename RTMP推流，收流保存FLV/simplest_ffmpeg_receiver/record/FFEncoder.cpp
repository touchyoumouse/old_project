#include "FFEncoder.h"

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
}

FFEncoder::FFEncoder()
    : ff_codec_(0)
{
    ff_codec_ctx_ = avcodec_alloc_context3(NULL);
}

FFEncoder::~FFEncoder()
{
    if (ff_codec_ctx_)
    {
        avcodec_free_context(&ff_codec_ctx_);
        //avcodec_close(ff_codec_ctx_);
        //av_free(&ff_codec_ctx_);
    }
}

//////////////////////////////////////////////////////////////////////////

FFVideoEncoder::FFVideoEncoder(int videoCodecId)
{
    ff_codec_ = avcodec_find_encoder((AVCodecID)videoCodecId);
//     avcodec_free_context(&ff_codec_ctx_);
//     ff_codec_ctx_ = avcodec_alloc_context3(ff_codec_);

    ff_yuv_frame_ = av_frame_alloc();
    ff_yuv_buf_ = 0;

    ff_rgb_frame_ = av_frame_alloc();
    ff_rgb_buf_ = 0;

    ff_sws_ctx_ = 0;
}

FFVideoEncoder::~FFVideoEncoder()
{
    if (ff_yuv_frame_) av_frame_free(&ff_yuv_frame_);
    delete[] ff_yuv_buf_;

    if (ff_rgb_frame_) av_frame_free(&ff_rgb_frame_);
    delete[] ff_rgb_buf_;

    if (ff_sws_ctx_) sws_freeContext(ff_sws_ctx_);
}

void FFVideoEncoder::Init(int width, int height, int fps, int bitRate)
{
    ff_codec_ctx_->width = width;
    ff_codec_ctx_->height = height;
    ff_codec_ctx_->pix_fmt = PIX_FMT_YUV420P;
    ff_codec_ctx_->time_base = av_make_q(1, fps);

    ff_codec_ctx_->max_b_frames = 0;
    ff_codec_ctx_->gop_size = 5*fps;

    ff_codec_ctx_->bit_rate = bitRate*1500;
    ff_codec_ctx_->bit_rate_tolerance = ff_codec_ctx_->bit_rate/10;
    ff_codec_ctx_->qmin = 1;
    ff_codec_ctx_->qmax = 3;
    ff_codec_ctx_->qblur = 0;
    ff_codec_ctx_->rc_qsquish = 1;

#if 0
    //ff_codec_ctx_->qmin = 1;
    //ff_codec_ctx_->qmax = 1;
    ff_codec_ctx_->pix_fmt = PIX_FMT_YUVJ420P;
#endif
    //ff_codec_ctx_->codec_type = AVMEDIA_TYPE_VIDEO;

    // 创建RGB32-->YUV转换器
    ff_sws_ctx_ = sws_getContext(
        ff_codec_ctx_->width, ff_codec_ctx_->height, AV_PIX_FMT_RGB32,
        ff_codec_ctx_->width, ff_codec_ctx_->height, ff_codec_ctx_->pix_fmt,
        SWS_BICUBIC, 0, 0, 0);

    // 申请内存数据
    AllocFrameBuf();

    // 打开编码器
    int ret = avcodec_open2(ff_codec_ctx_, ff_codec_, NULL);
    char buf[1024];
    av_make_error_string(buf, 1024, ret);
    int a = 0;
}

void FFVideoEncoder::AllocFrameBuf()
{
    ff_yuv_buf_size_ = avpicture_get_size(ff_codec_ctx_->pix_fmt, 
        ff_codec_ctx_->width, ff_codec_ctx_->height);
    ff_yuv_buf_ = new unsigned char[ff_yuv_buf_size_];
    avpicture_fill((AVPicture *)ff_yuv_frame_, ff_yuv_buf_, ff_codec_ctx_->pix_fmt, 
        ff_codec_ctx_->width, ff_codec_ctx_->height);

    ff_rgb_buf_size_ = avpicture_get_size(AV_PIX_FMT_RGB32, 
        ff_codec_ctx_->width, ff_codec_ctx_->height);
    ff_rgb_buf_ = new unsigned char[ff_rgb_buf_size_];
    avpicture_fill((AVPicture *)ff_rgb_frame_, ff_rgb_buf_, AV_PIX_FMT_RGB32, 
        ff_codec_ctx_->width, ff_codec_ctx_->height);
}

int FFVideoEncoder::Encode(const char* srcBuf, int srcLen, 
    char* outBuf, int& outLen, bool& isKeyframe)
{
    // 将输出地址和ff_rgb_frame_绑定
    avpicture_fill((AVPicture *)ff_rgb_frame_, (const uint8_t*)srcBuf, AV_PIX_FMT_RGB32, 
        ff_codec_ctx_->width, ff_codec_ctx_->height);

    // 转换为YUV
    sws_scale(ff_sws_ctx_, ff_rgb_frame_->data, ff_rgb_frame_->linesize, 0, 
        ff_codec_ctx_->height, ff_yuv_frame_->data, ff_yuv_frame_->linesize);

    AVPacket av_pakt;
    av_init_packet(&av_pakt);
    av_pakt.data = (uint8_t*)outBuf;
    av_pakt.size = outLen;

    int got_pic = 0;
    int ret = avcodec_encode_video2(ff_codec_ctx_, &av_pakt, ff_yuv_frame_, &got_pic);
    if (ret == 0 && got_pic)
    {
        int is_keyframe = ff_codec_ctx_->coded_frame->key_frame;
        int pic_type = ff_codec_ctx_->coded_frame->pict_type;

        isKeyframe = is_keyframe ? true : false;
        outLen = av_pakt.size;
    }
    else
    {
        outLen = 0;
    }

    av_free_packet(&av_pakt);

    return ret;
}

int FFVideoEncoder::EncodeYuv(const char* srcBuf, int srcLen, 
    char* outBuf, int& outLen, bool& isKeyframe)
{
    //memcpy(ff_yuv_buf_, srcBuf, srcLen);
    avpicture_fill((AVPicture *)ff_yuv_frame_, (const uint8_t*)srcBuf, ff_codec_ctx_->pix_fmt, 
        ff_codec_ctx_->width, ff_codec_ctx_->height);

    outLen = avcodec_encode_video(ff_codec_ctx_, (uint8_t*)outBuf, outLen,  ff_yuv_frame_);

    int is_keyframe = ff_codec_ctx_->coded_frame->key_frame;
    int pic_type = ff_codec_ctx_->coded_frame->pict_type;

    isKeyframe = is_keyframe ? true : false;

    return 0;
}

//////////////////////////////////////////////////////////////////////////

FFAudioEncoder::FFAudioEncoder(int audioCodecId)
{
    ff_codec_ = avcodec_find_encoder((AVCodecID)audioCodecId);

    bufsize_ = 0;
    input_frame_ = NULL;
}

FFAudioEncoder::~FFAudioEncoder()
{
    if (input_frame_)
    {
        av_frame_free(&input_frame_);
    }
}

void FFAudioEncoder::Init(int sampleRate, int channels, int bitRate)
{
    ff_codec_ctx_->sample_rate = sampleRate;
    ff_codec_ctx_->channels = channels;

    ff_codec_ctx_->sample_fmt = AV_SAMPLE_FMT_S16;
    ff_codec_ctx_->bit_rate = bitRate;

    ff_codec_ctx_->codec_type = AVMEDIA_TYPE_AUDIO;

    int err = avcodec_open2(ff_codec_ctx_, ff_codec_, NULL);
    int a = 0;

    input_frame_ = av_frame_alloc();
    input_frame_->nb_samples     = ff_codec_ctx_->frame_size;
    input_frame_->format         = ff_codec_ctx_->sample_fmt;
    input_frame_->channel_layout = ff_codec_ctx_->channel_layout;

    bufsize_ = av_samples_get_buffer_size(NULL,
        ff_codec_ctx_->channels, ff_codec_ctx_->frame_size,
        ff_codec_ctx_->sample_fmt, 0);
}

int FFAudioEncoder::BufSize()
{
    if (bufsize_)
        return bufsize_;
    return ff_codec_ctx_->frame_size * ff_codec_ctx_->channels * 2;
}

int FFAudioEncoder::Encode(unsigned char* buf, int bufLen, 
    unsigned char* outBuf, int& outLen)
{
    avcodec_fill_audio_frame(input_frame_,
        ff_codec_ctx_->channels, ff_codec_ctx_->sample_fmt,
        (const uint8_t*)buf, bufLen, 0);

    AVPacket av_pakt;
    av_init_packet(&av_pakt);
    av_pakt.data = outBuf;
    av_pakt.size = outLen;

    int is_got_out = 0;
    int ret = avcodec_encode_audio2(ff_codec_ctx_, &av_pakt, input_frame_, &is_got_out);
    if (is_got_out)
    {
        outLen = av_pakt.size;
    }
    else
    {
        outLen = 0;
    }

    av_free_packet(&av_pakt);

    return 0;

//     unsigned char* pinbuf = buf;
//     unsigned char* poutbuf = outBuf;
//     unsigned int remain_len = bufLen;
//     unsigned int remain_out_len = outLen;
// 
//     while (remain_len)
//     {
//         int write_len = avcodec_encode_audio(ff_codec_ctx_, poutbuf, remain_len, (short*)pinbuf);
//         //ff_codec_ctx_->codec->encode(ff_codec_ctx_, poutbuf, remain_len, pinbuf);
// 
//         remain_len -= BufSize();
//         pinbuf += BufSize();
// 
//         poutbuf += write_len;
//         remain_out_len -= write_len;
//     }

    return 0;
}

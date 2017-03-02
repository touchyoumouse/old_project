#include "FFDecoder.h"

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
}

FFDecoder::FFDecoder(AVCodecContext* codecContex)
    : ff_codec_(0)
{
    if (codecContex)
    {
        ff_codec_ctx_ = codecContex;
        is_ctx_need_free_ = false;
    }
    else
    {
        ff_codec_ctx_ = avcodec_alloc_context3(NULL);
        is_ctx_need_free_ = true;
    }
}

FFDecoder::~FFDecoder()
{
    if (is_ctx_need_free_ && ff_codec_ctx_)
    {
       // avcodec_free_context(&ff_codec_ctx_);
        avcodec_close(ff_codec_ctx_);
        av_free(ff_codec_ctx_);
    }
}

//////////////////////////////////////////////////////////////////////////

FFVideoDecoder::FFVideoDecoder(AVCodecContext* codecContex)
    : FFDecoder(codecContex)
{
    ff_codec_ = avcodec_find_decoder(codecContex->codec_id);

    ff_decoded_frame_ = av_frame_alloc();
    ff_decoded_buf_ = 0;

    ff_deinterlace_frame_ = av_frame_alloc();
    ff_deinterlace_buf_ = 0;

    ff_out_frame_ = av_frame_alloc();

    ff_sws_ctx_ = 0;
    out_width_ = 0;
    out_height_ = 0;
}

FFVideoDecoder::FFVideoDecoder(int videoCodecId)
    : FFDecoder(NULL)
{
    ff_codec_ = avcodec_find_decoder((AVCodecID)videoCodecId);

    ff_decoded_frame_ = av_frame_alloc();
    ff_decoded_buf_ = 0;

    do_deinterlace_ = false;
    ff_deinterlace_frame_ = av_frame_alloc();
    ff_deinterlace_buf_ = 0;

    ff_out_frame_ = av_frame_alloc();

    ff_sws_ctx_ = 0;
    out_width_ = 0;
    out_height_ = 0;
}

FFVideoDecoder::~FFVideoDecoder()
{
    FreeFrameBuf();

    if (ff_decoded_frame_) av_frame_free(&ff_decoded_frame_);
    
    do_deinterlace_ = false;
    if (ff_deinterlace_frame_) av_frame_free(&ff_deinterlace_frame_);

    if (ff_out_frame_) av_frame_free(&ff_out_frame_);

    if (ff_sws_ctx_) sws_freeContext(ff_sws_ctx_);
}

void FFVideoDecoder::Init(int width, int height, int inPixFmt,
    int outWidth, int outHeight)
{
    if (outWidth <= 0)
        outWidth = width;
    if (outHeight <= 0)
        outHeight = height;
    if (inPixFmt < 0)
        inPixFmt = AV_PIX_FMT_YUV420P;

    out_width_ = outWidth;
    out_height_ = outHeight;

    if (is_ctx_need_free_)
    {
        ff_codec_ctx_->width = width;
        ff_codec_ctx_->height = height;
        ff_codec_ctx_->pix_fmt = (AVPixelFormat)inPixFmt;
        ff_codec_ctx_->codec_type = AVMEDIA_TYPE_VIDEO;
    }

    // yuv420 --> rgb32
    ff_sws_ctx_ = sws_getContext(
        ff_codec_ctx_->width, ff_codec_ctx_->height, ff_codec_ctx_->pix_fmt,
        out_width_, out_height_, PIX_FMT_RGB32,
        SWS_BICUBIC, 0, 0, 0);

    AllocFrameBuf();

    int ret = avcodec_open2(ff_codec_ctx_, ff_codec_, NULL);
    //FILE_LOG_DEBUG("avcodec_open: %d\n", ret);
}

void FFVideoDecoder::UpdateOutWH(int outWidth, int outHeight)
{
    out_width_ = outWidth;
    out_height_ = outHeight;

    if (ff_sws_ctx_) sws_freeContext(ff_sws_ctx_);

    // yuv420 --> rgb32
    ff_sws_ctx_ = sws_getContext(
        ff_codec_ctx_->width, ff_codec_ctx_->height, ff_codec_ctx_->pix_fmt,
        out_width_, out_height_, PIX_FMT_RGB32,
        SWS_BICUBIC, 0, 0, 0);

    FreeFrameBuf();
    AllocFrameBuf();
}

void FFVideoDecoder::AllocFrameBuf()
{
    ff_decoded_buf_size_ = avpicture_get_size(ff_codec_ctx_->pix_fmt, 
        ff_codec_ctx_->width, ff_codec_ctx_->height);
    ff_decoded_buf_ = new unsigned char[ff_decoded_buf_size_];
    avpicture_fill((AVPicture *)ff_decoded_frame_, ff_decoded_buf_, 
        ff_codec_ctx_->pix_fmt, ff_codec_ctx_->width, ff_codec_ctx_->height);

    ff_deinterlace_buf_ = new unsigned char[ff_decoded_buf_size_];
    avpicture_fill((AVPicture *)ff_deinterlace_frame_, ff_deinterlace_buf_, 
        ff_codec_ctx_->pix_fmt,  ff_codec_ctx_->width, ff_codec_ctx_->height);

    ff_out_buf_size_ = avpicture_get_size(PIX_FMT_RGB32, 
        out_width_, out_height_);
}

void FFVideoDecoder::FreeFrameBuf()
{
    delete[] ff_decoded_buf_;
    delete[] ff_deinterlace_buf_;
}

int FFVideoDecoder::DecodeToFrame(unsigned char* buf, unsigned int bufLen)
{
    int is_got_pic = 0;

    AVPacket av_pakt;
    av_init_packet(&av_pakt);
    av_pakt.data = buf;
    av_pakt.size = bufLen;

    int decode_len = avcodec_decode_video2(ff_codec_ctx_, ff_decoded_frame_, &is_got_pic, &av_pakt);

    if (decode_len >= 0 && is_got_pic)
    {
        return ff_decoded_buf_size_;
    }

    return 0;
}

int FFVideoDecoder::DecodeAVPacketToFrame(AVPacket* avPacket)
{
    int is_got_pic = 0;
    int decode_len = avcodec_decode_video2(ff_codec_ctx_, ff_decoded_frame_, &is_got_pic, avPacket);

    if (decode_len != avPacket->size)
    {
		//@feng  (⊙_⊙)什么鬼？
        int a = decode_len;
    }

    if (decode_len >= 0 && is_got_pic)
    {
        return ff_decoded_buf_size_;
    }

    return 0;
}

int FFVideoDecoder::Decode(unsigned char* buf, unsigned int bufLen, 
    unsigned char* outBuf, int& outLen)
{
#if 1
    outLen = 0;

    int is_got_pic = 0;
    AVPacket av_pakt;
    av_init_packet(&av_pakt);
    av_pakt.data = buf;
    av_pakt.size = bufLen;

    int decode_len = avcodec_decode_video2(ff_codec_ctx_, ff_decoded_frame_, &is_got_pic, &av_pakt);

    if (decode_len >= 0 && is_got_pic)
    {
        avpicture_fill((AVPicture *)ff_out_frame_, outBuf,
            PIX_FMT_BGR32, out_width_, out_height_);

        // 图像反转问题
//         ff_decoded_frame_->data[0] += ff_decoded_frame_->linesize[0] * (ff_codec_ctx_->height - 1);
//         ff_decoded_frame_->linesize[0] *= -1;                      
//         ff_decoded_frame_->data[1] += ff_decoded_frame_->linesize[1] * (ff_codec_ctx_->height / 2 - 1);
//         ff_decoded_frame_->linesize[1] *= -1;
//         ff_decoded_frame_->data[2] += ff_decoded_frame_->linesize[2] * (ff_codec_ctx_->height / 2 - 1);
//         ff_decoded_frame_->linesize[2] *= -1;

        sws_scale(ff_sws_ctx_, ff_decoded_frame_->data, ff_decoded_frame_->linesize, 0,
            ff_codec_ctx_->height, ff_out_frame_->data, ff_out_frame_->linesize);

        outLen = ff_out_buf_size_;
    }

    return 0;
#else
    outLen = 0;

    int is_got_pic = 0;
    AVPacket av_pakt;
    av_pakt.data = buf;
    av_pakt.size = bufLen;

    int decode_len = avcodec_decode_video2(ff_codec_ctx_, ff_decoded_frame_, &is_got_pic, &av_pakt);
    if (decode_len >= 0 && is_got_pic)
    {
        if (ff_decoded_frame_->interlaced_frame)  // do_deinterlace
        {
            if(avpicture_deinterlace(
                (AVPicture *)ff_deinterlace_frame_, 
                (AVPicture *)ff_decoded_frame_,
                ff_codec_ctx_->pix_fmt, 
                ff_codec_ctx_->width, ff_codec_ctx_->height) >= 0)
            {
//                 sws_scale(ff_sws_ctx_, ff_deinterlace_frame_->data, ff_deinterlace_frame_->linesize, 0,
//                     ff_codec_ctx_->height, ff_out_frame_->data, ff_out_frame_->linesize);
                memcpy(outBuf, ff_deinterlace_buf_, ff_decoded_buf_size_);
                outLen = ff_decoded_buf_size_;
            }
            else
            {
//                 sws_scale(ff_sws_ctx_, ff_decoded_frame_->data, ff_decoded_frame_->linesize, 0,
//                     ff_codec_ctx_->height, ff_out_frame_->data, ff_out_frame_->linesize);
                memcpy(outBuf, ff_decoded_buf_, ff_decoded_buf_size_);
                outLen = ff_decoded_buf_size_;
            }
        }
        else
        {
            memcpy(outBuf, ff_decoded_buf_, ff_decoded_buf_size_);
            outLen = ff_decoded_buf_size_;
        }
    }
    do_deinterlace_ = ff_decoded_frame_->interlaced_frame ? true : false;

    return 0;
#endif
}

int FFVideoDecoder::DecodeAVPacket(AVPacket* avPacket, 
    unsigned char* outBuf, int& outLen)
{
    outLen = 0;

    int is_got_pic = 0;

    int decode_len = avcodec_decode_video2(ff_codec_ctx_, ff_decoded_frame_, &is_got_pic, avPacket);

    if (decode_len >= 0 && is_got_pic)
    {
        avpicture_fill((AVPicture *)ff_out_frame_, outBuf,
            PIX_FMT_BGR32, out_width_, out_height_);

        sws_scale(ff_sws_ctx_, ff_decoded_frame_->data, ff_decoded_frame_->linesize, 0,
            ff_codec_ctx_->height, ff_out_frame_->data, ff_out_frame_->linesize);

        outLen = ff_out_buf_size_;
    }
    else
    {
        int a = 0;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////

FFAudioDecoder::FFAudioDecoder(AVCodecContext* codecContex)
    : FFDecoder(codecContex)
{
    ff_codec_ = avcodec_find_decoder(codecContex->codec_id);
    pcm_frame_ = av_frame_alloc();
}

FFAudioDecoder::FFAudioDecoder(int audioCodecId)
    : FFDecoder(NULL)
{
    ff_codec_ = avcodec_find_decoder((AVCodecID)audioCodecId);
    pcm_frame_ = av_frame_alloc();
}

FFAudioDecoder::~FFAudioDecoder()
{
    if (pcm_frame_)
    {
        av_frame_free(&pcm_frame_);
    }
}

void FFAudioDecoder::Init(int sampleRate, int channels)
{
    if (is_ctx_need_free_)
    {
        ff_codec_ctx_->sample_rate = sampleRate;
        ff_codec_ctx_->channels = channels;
        ff_codec_ctx_->sample_fmt = AV_SAMPLE_FMT_S16;
    }

    int err = avcodec_open2(ff_codec_ctx_, ff_codec_, NULL);

    sample_fmt_ = ff_codec_ctx_->sample_fmt;
    sample_rate_ = ff_codec_ctx_->sample_rate;
}

int FFAudioDecoder::BufSize()
{
    return ff_codec_ctx_->frame_size * ff_codec_ctx_->channels * 2;
}

int FFAudioDecoder::Decode(unsigned char* buf, unsigned int bufLen, 
    unsigned char* outBuf, int& outLen)
{
    unsigned char* pinbuf = buf;
    unsigned int remain_len = bufLen;

    unsigned char* poutbuf = outBuf;
    unsigned int remain_out_len = outLen;

    outLen = 0;

#if 0
    while (remain_len)
    {
        int write_len = remain_out_len;
        AVPacket av_pakt;
        av_init_packet(&av_pakt);
        av_pakt.data = pinbuf;
        av_pakt.size = remain_len;
        int used_len = avcodec_decode_audio3(ff_codec_ctx_, (short*)poutbuf, &write_len,
            &av_pakt);
        if (used_len <= 0)
        {
            break;
        }

        pinbuf += used_len;
        remain_len -= used_len;

        remain_out_len -= write_len;
        poutbuf += write_len;
    }
#else
    while (remain_len)
    {
        int is_got_frame = 0;

        AVPacket av_pakt;
        av_init_packet(&av_pakt);
        av_pakt.data = pinbuf;
        av_pakt.size = remain_len;
        int used_len = avcodec_decode_audio4(ff_codec_ctx_, pcm_frame_, 
            &is_got_frame, &av_pakt);
        if (used_len <= 0)
        {
            break;
        }

        if (is_got_frame)
        {
            int data_size = av_samples_get_buffer_size(NULL, ff_codec_ctx_->channels,
                pcm_frame_->nb_samples,
                ff_codec_ctx_->sample_fmt, 1);
            memcpy(poutbuf, pcm_frame_->data[0], data_size);

            remain_out_len -= data_size;
            poutbuf += data_size;
        }

        pinbuf += used_len;
        remain_len -= used_len;
    }
#endif

    outLen = (poutbuf - outBuf);

    return 0;
}

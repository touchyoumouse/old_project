/*******************************************************************************
 * ff_url_reader.cpp
 * Copyright: (c) 2013 Haibin Du(haibinnet@qq.com). All rights reserved.
 * -----------------------------------------------------------------------------
 *
 * -----------------------------------------------------------------------------
 * 2014-3-10 23:29 - Created (Haibin Du)
 ******************************************************************************/

#include "ff_url_reader.h"

extern "C" {
#include "libavutil/error.h"
}

#include "../base/simple_logger.h"

namespace {
int FFInterruptCallback(void* pdata)
{
    FFUrlReader* ffreader = (FFUrlReader*)pdata;

    if (ffreader->IsFFNeedInterrupt())
    {
        return 1;
    }

    return 0;
}
}

FFUrlReader::FFUrlReader(Observer* ob)
{
    observer_ = ob;

    ff_fmt_ctx_ = avformat_alloc_context();

    const AVIOInterruptCB int_cb = {FFInterruptCallback, this};
    ff_fmt_ctx_->interrupt_callback = int_cb;

    is_ff_need_interrupt_ = false;

    audio_stream_index_ = -1;
    video_stream_index_ = -1;

    width_ = 0;
    height_ = 0;
    sample_rate_ = 0;
    channel_count_ = 0;
    is_has_audio_ = false;
    is_has_video_ = false;

    is_opening_ = false;
    is_open_failed_ = false;
    is_reading_ = false;
}


FFUrlReader::~FFUrlReader()
{

}

void FFUrlReader::Start(const std::string& readUrl)
{
    read_url_ = readUrl;

    base::SimpleThread::ThreadStart();
}

void FFUrlReader::Stop()
{
    SIMPLE_LOG("\n");

    if (IsThreadStop()) return;

    SIMPLE_LOG("\n");

    base::SimpleThread::ThreadStop();

    //if (is_opening_)
    {
        is_ff_need_interrupt_ = true;
    }

    if (false == is_open_failed_)
        base::SimpleThread::ThreadJoin();
}

void FFUrlReader::ThreadRun()
{   
	if (false == FFOpenStream())
    {
        ThreadStop();

        return;
    }

    if (false == FFFindCodecInfo())
    {
        FFCloseStream();
        ThreadStop();

        return;
    }

    observer_->OnFFEvent(FF_URL_OPEN_SUCCEED);

    // 循环读取直到结束
    FFProcessPacket();

    FFCloseStream();

    while (false == base::SimpleThread::IsThreadStop())
    {
        MillsecSleep(100);
    }
}

bool FFUrlReader::FFOpenStream()
{
    is_opening_ = true;
    timeout_watch_.Reset();

    SIMPLE_LOG("(%p) try open url %s\n", this, read_url_.c_str());

    // 以tcp方式读取rtsp数据

    AVDictionary* ffoptions = NULL;
    av_dict_set(&ffoptions, "rtsp_transport", "tcp", 0);
    av_dict_set(&ffoptions, "fflags", "nobuffer", 0);

    int ret_val = avformat_open_input(&ff_fmt_ctx_, read_url_.c_str(), NULL, &ffoptions);

    av_dict_free(&ffoptions);

    char buf[1024];
    av_make_error_string(buf, 1024, ret_val);

    if (SimpleThread::IsThreadStop()) return false;

    if (ret_val < 0)
    {
        SIMPLE_LOG("(%p) open failed %s\n", this, read_url_.c_str());

        is_open_failed_ = true;
        if (observer_)
            observer_->OnFFEvent(FF_URL_OPEN_FAILED);

        return false;
    }

    SIMPLE_LOG("(%p) open succeed %s\n", this, read_url_.c_str());

    is_opening_ = false;
    timeout_watch_.Reset();

    return true;
}

void FFUrlReader::FFCloseStream()
{
    if (ff_fmt_ctx_)
    {
        avformat_close_input(&ff_fmt_ctx_);

        if (observer_)
            observer_->OnFFEvent(FF_URL_CLOSE);

        avformat_free_context(ff_fmt_ctx_);
        ff_fmt_ctx_ = NULL;
    }
}

bool FFUrlReader::FFFindCodecInfo()
{
    is_opening_ = true;
    timeout_watch_.Reset();

    //int ret_val = av_find_stream_info(ff_fmt_ctx_);
    int ret_val = avformat_find_stream_info(ff_fmt_ctx_, NULL);
    if (ret_val < 0)
    {
        SIMPLE_LOG("(%p) find info failed %s\n", this, read_url_.c_str());

        is_open_failed_ = true;
        if (observer_)
            observer_->OnFFEvent(FF_URL_OPEN_FAILED);

        return false;
    }

    SIMPLE_LOG("(%p) find info succeed %s\n", this, read_url_.c_str());

    if (SimpleThread::IsThreadStop()) return false;

    is_opening_ = false;
    timeout_watch_.Reset();

    for (unsigned int i = 0; i < ff_fmt_ctx_->nb_streams; ++i)
    {
        if (ff_fmt_ctx_->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            audio_stream_index_ = i;
        }
        else if (ff_fmt_ctx_->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            video_stream_index_ = i;
        }
    }

    char* audio_extra = NULL;
    int audio_extra_size = 0;
    AVCodecID audio_id = AV_CODEC_ID_NONE;

    AVCodecID video_id = AV_CODEC_ID_NONE;
    char* video_extra = NULL;
    int video_extra_size = 0;

    // 提取音频编码信息
    if (audio_stream_index_ >= 0)
    {
        AVCodecContext* audio_codec_ctx = NULL;
        audio_codec_ctx = ff_fmt_ctx_->streams[audio_stream_index_]->codec;

        audio_id = audio_codec_ctx->codec_id;

        sample_rate_ = audio_codec_ctx->sample_rate;
        channel_count_ = audio_codec_ctx->channels;

        if (sample_rate_ > 0 && channel_count_ > 0)
            is_has_audio_ = true;

        audio_extra = (char*)audio_codec_ctx->extradata;
        audio_extra_size = audio_codec_ctx->extradata_size;
    }

    // 提取视频编码信息
    AVPixelFormat pixfmt = AV_PIX_FMT_YUV420P;
    AVCodecContext* video_codec_ctx = NULL;
    if (video_stream_index_ >= 0)
    {
        video_codec_ctx = ff_fmt_ctx_->streams[video_stream_index_]->codec;

        video_id = video_codec_ctx->codec_id;

        width_ = video_codec_ctx->width;
        height_ = video_codec_ctx->height;
        pixfmt = video_codec_ctx->pix_fmt;

        is_has_video_ = true;

        video_extra = (char*)video_codec_ctx->extradata;
        video_extra_size = video_codec_ctx->extradata_size;
    }

    // 回调流编码信息
    observer_->OnFFCodecInfo(
        audio_id, sample_rate_, channel_count_,
        video_id, width_, height_, pixfmt,
        video_extra, video_extra_size);

    return true;
}

void FFUrlReader::FFProcessPacket()
{
    int audio_timebase = 0;
    int video_timebase = 0;

    if (audio_stream_index_ >= 0)
    {
        audio_timebase = ff_fmt_ctx_->streams[audio_stream_index_]->time_base.den;
    }
    if (video_stream_index_ >= 0)
    {
        video_timebase = ff_fmt_ctx_->streams[video_stream_index_]->time_base.den;
    }

    long long a_timestamp = 0;
    long long v_timestamp = 0;

    AVCodecContext* video_ctx = ff_fmt_ctx_->streams[video_stream_index_]->codec;
    //AVCodecParserContext* parser_ctx = av_parser_init(video_ctx->codec_id);

    bool has_got_keyframe = false;

    SIMPLE_LOG("(%p) begin reading %s\n", this, read_url_.c_str());

	//@feng 循环读取
    while (true)
    {
        AVPacket av_pakt;
        av_init_packet(&av_pakt);

        if (base::SimpleThread::IsThreadStop()) break;

        is_reading_ = true;
        if (av_read_frame(ff_fmt_ctx_, &av_pakt) < 0)
        {
            SIMPLE_LOG("(%p) read frame failed\n", this);

            is_reading_ = false;
            is_open_failed_ = true;

            observer_->OnFFEvent(FF_URL_ABORT);

            break;
        }

        is_reading_ = false;
        timeout_watch_.Reset();

        if (av_pakt.stream_index == audio_stream_index_)
        {
            // 音频数据
            if (av_pakt.size > 0 && has_got_keyframe)
            {
                long long duration = av_pakt.duration * 1000.0 / audio_timebase;

                if (av_pakt.pts > 0)
                {
                    a_timestamp = av_rescale(av_pakt.pts, 1000, audio_timebase);
                }
                observer_->OnFFAudioBuf((char*)av_pakt.data, av_pakt.size,
                    a_timestamp, duration);
                a_timestamp += duration;
            }
        }
        else if (av_pakt.stream_index == video_stream_index_)
        {
            //SIMPLE_LOG("(%p) read video frame, size: %d\n", this, av_pakt.size);

            // 视频数据
            if (false == has_got_keyframe)
            {
                has_got_keyframe = (av_pakt.flags & AV_PKT_FLAG_KEY);
            }

            if (av_pakt.size > 0 && has_got_keyframe)
            {
                long long duration = av_pakt.duration * 1000.0 / video_timebase;
                
                //duration *= 2;
                if (av_pakt.pts > 0)
                {
                    v_timestamp = av_rescale(av_pakt.pts, 1000, video_timebase);
                }
                observer_->OnFFVideoBuf((char*)av_pakt.data, av_pakt.size, v_timestamp, duration);
                //observer_->OnFFAVPacket(&av_pakt, v_timestamp, duration);
                v_timestamp += duration;
            }
        }
        else
        {
            //av_free_packet(&av_pakt);
        }
        av_free_packet(&av_pakt);
    }
    //av_free_packet(&av_pakt);

    SIMPLE_LOG("(%p) thread end\n", this);
}

bool FFUrlReader::IsFFNeedInterrupt()
{
    return is_ff_need_interrupt_;
}

bool FFUrlReader::IsConnectError()
{
    // 判断rtsp
    if (is_opening_)
    {
        if (timeout_watch_.Get() >= 5000)
        {
            SIMPLE_LOG("(%p) open time out\n", this);

            return true;
        }
        else
        {
            return false;
        }
    }

    if (is_open_failed_)
    {
        return true;
    }

    if (false == IsThreadStop() && is_reading_ && timeout_watch_.Get() >= 5000)
    {
        SIMPLE_LOG("(%p) reading time out\n", this);

        return true;
    }

    return false;
}


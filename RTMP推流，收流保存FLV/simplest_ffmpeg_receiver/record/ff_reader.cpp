/*******************************************************************************
 * ff_reader.cpp
 * Copyright: (c) 2014 Haibin Du(haibindev.cnblogs.com). All rights reserved.
 * -----------------------------------------------------------------------------
 *
 * -----------------------------------------------------------------------------
 * 2015-9-7 19:36 - Created (Haibin Du)
 ******************************************************************************/

#include "ff_reader.h"

extern "C" {
#include "libavutil/error.h"
}

#include "base/simple_logger.h"

FFReader::FFReader()
{
    ff_fmt_ctx_ = avformat_alloc_context();

    audio_stream_index_ = -1;
    video_stream_index_ = -1;
    audio_id_ = AV_CODEC_ID_NONE;
    video_id_ = AV_CODEC_ID_NONE;

    bsfc_ = av_bitstream_filter_init("h264_mp4toannexb");
    is_picture_ = false;

    audio_timebase_ = 0;
    video_timebase_ = 0;
    a_timestamp_ = 0;
    v_timestamp_ = 0;

    width_ = 0;
    height_ = 0;
    sample_rate_ = 0;
    channel_count_ = 0;
    audio_duration_ = 0;
    video_duration_ = 0;

    is_has_audio_ = false;
    is_has_video_ = false;
}


FFReader::~FFReader()
{
    Close();
}

bool FFReader::Open(const std::string& readFile)
{
    filename_ = readFile;

    if (false == FFOpenStream())
    {
        return false;
    }

    if (false == FFFindCodecInfo())
    {
        return false;
    }

    return true;
}

void FFReader::Close()
{
    if (ff_fmt_ctx_)
    {
        SIMPLE_LOG("\n");
        av_bitstream_filter_close(bsfc_);

        SIMPLE_LOG("\n");
        avformat_close_input(&ff_fmt_ctx_);

        SIMPLE_LOG("\n");
        avformat_free_context(ff_fmt_ctx_);

        ff_fmt_ctx_ = NULL;
    }
}


char* FFReader::ReadFrame(int* readType, int* readSize, long long* frameTime,
    bool* isKeyframe)
{
    *readSize = 0;
    *readType = 0;

    av_init_packet(&reading_pakt_);

    if (av_read_frame(ff_fmt_ctx_, &reading_pakt_) < 0)
    {
        *readType = -1;

        return NULL;
    }

    bool has_got_keyframe = false;
    char* ret_buf = NULL;

    if (reading_pakt_.stream_index == audio_stream_index_)
    {
        // 音频数据
        if (reading_pakt_.size > 0/* && has_got_keyframe*/)
        {
            long long duration = reading_pakt_.duration * 1000.0 / audio_timebase_;

            *readType = 1;

            ret_buf = (char*)reading_pakt_.data;
            *readSize = reading_pakt_.size;
            *frameTime = a_timestamp_;

            a_timestamp_ += duration;

            a_timestamp_ = av_rescale_q(reading_pakt_.pts + reading_pakt_.duration,
                ff_fmt_ctx_->streams[audio_stream_index_]->time_base, av_make_q(1, 1000));
        }
    }
    else if (reading_pakt_.stream_index == video_stream_index_)
    {
        AVStream* video_stream = ff_fmt_ctx_->streams[video_stream_index_];

        // 视频数据
        if (false == has_got_keyframe)
        {
            has_got_keyframe = (reading_pakt_.flags & AV_PKT_FLAG_KEY);
        }

        if (reading_pakt_.size > 0/* && has_got_keyframe*/)
        {
            long long duration = reading_pakt_.duration * 1000.0 / video_timebase_;
            //duration *= 2;

            *readType = 2;

            ret_buf = (char*)reading_pakt_.data;
            *readSize = reading_pakt_.size;
            *frameTime = v_timestamp_;

            if (video_id_ == AV_CODEC_ID_H264)
            {
                v_timestamp_ += duration;

                av_bitstream_filter_filter(bsfc_, video_stream->codec, NULL,
                    &reading_pakt_.data, &reading_pakt_.size, reading_pakt_.data, reading_pakt_.size, 0);

                ret_buf = (char*)reading_pakt_.data;
                *readSize = reading_pakt_.size;
            }
            else
            {
                v_timestamp_ = av_rescale_q(reading_pakt_.pts + reading_pakt_.duration,
                    ff_fmt_ctx_->streams[video_stream_index_]->time_base, av_make_q(1, 1000));
            }

            if (isKeyframe)
            {
                *isKeyframe = ((reading_pakt_.flags & AV_PKT_FLAG_KEY) == AV_PKT_FLAG_KEY);
            }
        }
    }

    return ret_buf;
}

void FFReader::FreeFrame()
{
    av_free_packet(&reading_pakt_);
}

bool FFReader::FFOpenStream()
{
    int ret_val = avformat_open_input(&ff_fmt_ctx_, filename_.c_str(), NULL, NULL);

    char buf[1024];
    av_make_error_string(buf, 1024, ret_val);

    if (ret_val < 0)
    {
        return false;
    }

    return true;
}

bool FFReader::FFFindCodecInfo()
{
    //int ret_val = av_find_stream_info(ff_fmt_ctx_);
    int ret_val = avformat_find_stream_info(ff_fmt_ctx_, NULL);
    if (ret_val < 0)
    {
        return false;
    }

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

    // 提取音频编码信息
    if (audio_stream_index_ >= 0)
    {
        AVStream* audio_stream = ff_fmt_ctx_->streams[audio_stream_index_];
        AVCodecContext* audio_codec_ctx = audio_stream->codec;

        audio_id_ = audio_codec_ctx->codec_id;

        sample_rate_ = audio_codec_ctx->sample_rate;
        channel_count_ = audio_codec_ctx->channels;

        if (sample_rate_ > 0 && channel_count_ > 0)
            is_has_audio_ = true;

        audio_extra = (char*)audio_codec_ctx->extradata;
        audio_extra_size = audio_codec_ctx->extradata_size;

        audio_timebase_ = ff_fmt_ctx_->streams[audio_stream_index_]->time_base.den;
        if (audio_stream->duration > 0)
        {
            audio_duration_ = av_rescale_q(audio_stream->duration,
                audio_stream->time_base, av_make_q(1, 1000));
        }
        else
        {
            audio_duration_ = audio_stream->nb_frames * 1000 / audio_codec_ctx->sample_rate;
        }
    }

    // 提取视频编码信息
    AVPixelFormat pixfmt = AV_PIX_FMT_YUV420P;
    if (video_stream_index_ >= 0)
    {
        AVStream* video_stream = ff_fmt_ctx_->streams[video_stream_index_];
        AVCodecContext* video_codec_ctx = video_stream->codec;

        video_id_ = video_codec_ctx->codec_id;

        width_ = video_codec_ctx->width;
        height_ = video_codec_ctx->height;
        pixfmt = video_codec_ctx->pix_fmt;

        is_has_video_ = true;

        video_timebase_ = ff_fmt_ctx_->streams[video_stream_index_]->time_base.den;
        video_duration_ = av_rescale_q(video_stream->duration,
            video_stream->time_base, av_make_q(1, 1000));

        is_picture_ = (video_stream->nb_frames == 1);
    }

    return true;
}

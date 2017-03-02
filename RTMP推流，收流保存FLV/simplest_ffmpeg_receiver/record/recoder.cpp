/*******************************************************************************
 * recoder.cpp
 * Copyright: (c) 2014 Haibin Du(haibindev.cnblogs.com). All rights reserved.
 * -----------------------------------------------------------------------------
 *
 * -----------------------------------------------------------------------------
 * 2015-9-5 14:14 - Created (Haibin Du)
 ******************************************************************************/

#include "recoder.h"

#include "base/byte_stream.h"
#include "base/h264_frame_parser.h"
#include "rtmp_base/flv_writter.h"
#include "record/FFWritter.h"
#include "record/avi_muxer.h"

#include "base/simple_logger.h"

Recorder::Recorder(SaveFileType fileType, const std::string& fileName)
    : file_type_(fileType), file_name_(fileName)
{
    samrate_ = 0;
    channel_ = 0;
    width_ = 0;
    height_ = 0;
    fps_ = 0;
    bitrate_ = 0;

    opening_millsecs_ = 0;
    ending_millsecs_ = 0;

    avi_muxer_ = NULL;
    mp4_writter_ = NULL;
    flv_writter_ = NULL;
    x264_buf_ = NULL;
    x264_len_ = 0;
    sps_ = pps_ = NULL;
    sps_size_ = pps_size_ = 0;
    aac_frame_count_ = 0;

    is_pause_ = false;
}

Recorder::~Recorder()
{

}

void Recorder::SetOpeningFile(const std::string& filename,
    int opendingMillsecs)
{
    opening_name_ = filename;
    opening_millsecs_ = opendingMillsecs;
}

void Recorder::SetEndingFile(const std::string& filename,
    int endingMillsecs)
{
    ending_name_ = filename;
    ending_millsecs_ = endingMillsecs;
}

void Recorder::Start(bool isHasAudio, int sampleRate, int channel,
    bool isHasVideo, int width, int height, int fps, int bitrate)
{
    samrate_ = sampleRate;
    channel_ = channel;
    width_ = width;
    height_ = height;
    fps_ = fps;
    bitrate_ = bitrate;

    CreateSaveFiles(isHasAudio, isHasVideo);
}

void Recorder::Stop()
{
    AppendEnding();

    if (avi_muxer_)
    {
        delete avi_muxer_;
        avi_muxer_ = NULL;
    }

    if (mp4_writter_)
    {
        delete mp4_writter_;
        mp4_writter_ = NULL;
    }

    if (flv_writter_)
    {
        delete flv_writter_;
        flv_writter_ = NULL;
    }

    delete[] x264_buf_;
    delete[] sps_;
    delete[] pps_;
}

void Recorder::Pause()
{
    is_pause_ = !is_pause_;

    if (avi_muxer_)
    {
        avi_muxer_->SetPause(is_pause_);
    }

    if (mp4_writter_)
        mp4_writter_->SetPause(is_pause_);

    if (flv_writter_)
    {
        flv_writter_->SetPause(is_pause_, time_counter_.Get());
    }
}

void Recorder::CreateSaveFiles(bool isHasAudio, bool isHasVideo)
{
    std::string::size_type extpos1 = file_name_.rfind(".mp4");
    std::string::size_type extpos2 = file_name_.rfind(".flv");
    if (extpos1 != std::string::npos || extpos2 != std::string::npos)
    {
        file_name_ = file_name_.substr(0, file_name_.size()-4);
    }

    if (file_type_ & SAVE_FILE_MP4)
    {
        mp4_writter_ = new FFWritter(file_name_+".mp4", "mp4");
        mp4_writter_->Open(isHasVideo, AV_CODEC_ID_H264, width_, height_, fps_, bitrate_,
            isHasAudio, AV_CODEC_ID_AAC, samrate_, channel_);
    }

    if (file_type_ & SAVE_FILE_FLV)
    {
        SIMPLE_LOG("create flv: %s\n", (file_name_+".flv").c_str());

        flv_writter_ = new FlvWritter();
        flv_writter_->Open((file_name_+".flv").c_str());
        flv_writter_->WriteFlvHeader(isHasAudio, isHasVideo);
        if (isHasAudio)
        {
            flv_writter_->WriteAACSequenceHeaderTag(samrate_, channel_);
        }

        x264_buf_ = new char[width_*height_*3];
        x264_len_ = 0;
        sps_ = new char[1024];
        sps_size_ = 0;
        pps_ = new char[1024];
        pps_size_ = 0;
    }
//@feng 添加片头
    AppendOpening();

    time_counter_.Reset();
}

void Recorder::OnH264Extra(const char* extraBuf, int extraSize)
{
    if (mp4_writter_)
        mp4_writter_->SetVideoExtraData((char*)extraBuf, extraSize);
}

//@feng 存入文件
void Recorder::OnH264Frame(const char* frameBuf, int frameSize,
    bool isKeyframe, long long shouldTS)
{
    if (is_pause_) return;

    SIMPLE_LOG("shouldTS: %lld, timecounter: %lld\n", shouldTS, time_counter_.Get());
    if (mp4_writter_)
        mp4_writter_->WriteVideoEncodedData((char*)frameBuf, frameSize,
        isKeyframe, time_counter_.Get()*1000);

    if (flv_writter_)
    {
        bool is_keyframe = false;
        bool is_first = false;
        if (sps_size_ == 0 || pps_size_ == 0)
        {
            is_first = true;
            ParseH264Frame((char*)frameBuf, frameSize, x264_buf_, x264_len_,
                sps_, sps_size_, pps_, pps_size_, is_keyframe, NULL, NULL);
        }
        else
        {
            ParseH264Frame((char*)frameBuf, frameSize, x264_buf_, x264_len_,
                NULL, sps_size_, NULL, pps_size_, is_keyframe, NULL, NULL);
        }

        if (is_first && sps_size_ && pps_size_)
        {
            SIMPLE_LOG("find sps and pps: %s\n", (file_name_+".flv").c_str());

            flv_writter_->WriteAVCSequenceHeaderTag(sps_, sps_size_,
                pps_, pps_size_);
        }

        if (sps_size_ && pps_size_)
        {
            flv_writter_->WriteAVCData(x264_buf_, x264_len_,
                time_counter_.Get(), isKeyframe);
        }
        else
        {
            SIMPLE_LOG("not write: %s\n", (file_name_+".flv").c_str());
        }
    }
}

void Recorder::OnAACExtra(const char* extraBuf, int extraSize)
{
    if (mp4_writter_)
        mp4_writter_->SetAudioExtraData((char*)extraBuf, extraSize);
}

void Recorder::OnAACFrame(const char* frameBuf, int frameSize)
{
    if (is_pause_) return;

    long long delay_millsecs = 100;

    long long audio_pts = av_rescale(aac_frame_count_, 1000, samrate_);
    aac_frame_count_ += 1024;

    if (mp4_writter_)
        mp4_writter_->WriteAudioEncodedData((char*)frameBuf, frameSize, delay_millsecs);

    if (flv_writter_ && sps_size_ && pps_size_)
    {
        flv_writter_->WriteAACData((char*)frameBuf, frameSize,
            time_counter_.Get()+delay_millsecs);
    }
}

void Recorder::AppendOpening()
{
    Appending(true);
}

void Recorder::AppendEnding()
{
    Appending(false);
}

void Recorder::Appending(bool isOpening)
{
    std::string appending_name = isOpening ? opening_name_ : ending_name_;

    FFReader reader;
    if (reader.Open(appending_name))
    {
        if (reader.IsPicture())
        {
            AppendingPicture(reader, isOpening);
        }
        else
        {
            AppendingVideo(reader, isOpening);
        }

        reader.Close();
    }
}

void Recorder::AppendingVideo(FFReader& ffReader, bool isOpening)
{
    int data_size = 0;
    int data_type = -1;
    long long data_time = 0;
    for (;;)
    {
        bool iskeyframe = false;
        char* data_buf = ffReader.ReadFrame(&data_type, &data_size, &data_time, &iskeyframe);
        if (ffReader.AudioCodecId() == AV_CODEC_ID_NONE) // 没有音频的情况
        {
            if (data_type < 0)
            {
                break;
            }
        }
        else
        {
            if (data_type < 0/* || data_time > ffReader.AudioDuration()*/)
            {
                break;
            }
        }

        if (data_type == 1)         // 音频
        {
            if (isOpening)
            {
                if (mp4_writter_)
                    mp4_writter_->WriteAudioOpeningData(data_buf, data_size, data_time);
                if (flv_writter_)
                    flv_writter_->WriteAudioOpeningData(data_buf, data_size, data_time);
            }
            else
            {
                if (mp4_writter_)
                    mp4_writter_->WriteAudioEndingData(data_buf, data_size, data_time);
                if (flv_writter_)
                    flv_writter_->WriteAudioEndingData(data_buf, data_size, data_time);
            }
        }
        else if (data_type == 2)    // 视频
        {
            if (isOpening)
            {
                if (mp4_writter_)
                    mp4_writter_->WriteVideoOpeningData(data_buf, data_size, data_time);
                if (flv_writter_)
                {
                    bool is_keyframe = false;
                    char tmpspsbuf[256], tmpppsbuf[128];
                    int tmpspssize = 0; int tmpppssize = 0;

                    ParseH264Frame((char*)data_buf, data_size, x264_buf_, x264_len_,
                        tmpspsbuf, tmpspssize, tmpppsbuf, tmpppssize,
                        is_keyframe, NULL, NULL);

                    if (tmpspssize && tmpppssize)
                    {
                        flv_writter_->WriteAVCSequenceHeaderTag(tmpspsbuf, tmpspssize,
                            tmpppsbuf, tmpppssize);
                    }

                    flv_writter_->WriteVideoOpeningData(x264_buf_, x264_len_, data_time, iskeyframe);
                }
            }
            else
            {
                if (mp4_writter_)
                    mp4_writter_->WriteVideoEndingData(data_buf, data_size, data_time);
                if (flv_writter_)
                {
                    bool is_keyframe = false;
                    char tmpspsbuf[256], tmpppsbuf[128];
                    int tmpspssize = 0; int tmpppssize = 0;

                    ParseH264Frame((char*)data_buf, data_size, x264_buf_, x264_len_,
                        tmpspsbuf, tmpspssize, tmpppsbuf, tmpppssize,
                        is_keyframe, NULL, NULL);

                    if (tmpspssize && tmpppssize)
                    {
                        flv_writter_->WriteAVCSequenceHeaderTag(tmpspsbuf, tmpspssize,
                            tmpppsbuf, tmpppssize);
                    }

                    flv_writter_->WriteVideoEndingData(x264_buf_, x264_len_, data_time, iskeyframe);
                }
            }
        }

        ffReader.FreeFrame();
    }

    if (ffReader.AudioCodecId() == AV_CODEC_ID_NONE)
    {
        // 附加空音频
        AppendingZeroPCM(ffReader.VideoDuration(), isOpening);
    }

    if (isOpening)
    {
        if (ffReader.AudioCodecId() == AV_CODEC_ID_NONE)    // 没有音频时
        {
            if (mp4_writter_)
                mp4_writter_->SetOpeningDuration(ffReader.VideoDuration(), ffReader.VideoDuration());
            if (flv_writter_)
                flv_writter_->SetOpeningDuration(ffReader.VideoDuration());
        }
        else
        {
            if (mp4_writter_)
                mp4_writter_->SetOpeningDuration(ffReader.AudioDuration(), ffReader.VideoDuration());
            if (flv_writter_)
                flv_writter_->SetOpeningDuration(ffReader.AudioDuration());
        }
    }
}

void Recorder::AppendingPicture(FFReader& ffReader, bool isOpening)
{
    int data_size = 0;
    int data_type = -1;
    long long data_time = 0;
    char* data_buf = ffReader.ReadFrame(&data_type, &data_size, &data_time);
//     if (data_type < 0 || data_time > ffReader.AudioDuration())
//     {
//         return;
//     }

    // 为flv处理数据
    if (flv_writter_)
    {
        bool is_keyframe = false;
        char tmpspsbuf[256], tmpppsbuf[128];
        int tmpspssize = 0; int tmpppssize = 0;

        ParseH264Frame((char*)data_buf, data_size, x264_buf_, x264_len_,
            tmpspsbuf, tmpspssize, tmpppsbuf, tmpppssize,
            is_keyframe, NULL, NULL);

        if (tmpspssize && tmpppssize)
        {
            flv_writter_->WriteAVCSequenceHeaderTag(tmpspsbuf, tmpspssize,
                tmpppsbuf, tmpppssize);
        }
    }

    // 按帧率写入重复图片
    int repeat_count = opening_millsecs_ * fps_ / 1000;
    for (int i = 0; i < repeat_count; ++i)
    {
        long long timestamp = i * 1000 / fps_;

        if (isOpening)
        {
            if (mp4_writter_)
                mp4_writter_->WriteVideoOpeningData(data_buf, data_size, timestamp);
            if (flv_writter_)
                flv_writter_->WriteVideoOpeningData(x264_buf_, x264_len_, data_time, true);
        }
        else
        {
            if (mp4_writter_)
                mp4_writter_->WriteVideoEndingData(data_buf, data_size, timestamp);
            if (flv_writter_)
                flv_writter_->WriteVideoOpeningData(x264_buf_, x264_len_, data_time, true);
        }
    }

    ffReader.FreeFrame();

    // 附加空音频
    AppendingZeroPCM(opening_millsecs_, isOpening);

    if (isOpening)
    {
        if (mp4_writter_)
            mp4_writter_->SetOpeningDuration(opening_millsecs_, opening_millsecs_);
        if (flv_writter_)
            flv_writter_->SetOpeningDuration(opening_millsecs_);
    }
}

void Recorder::AppendingZeroPCM(long long pcmMillsecs, bool isOpening)
{
    int sample_count = (long long)pcmMillsecs*samrate_/1024000;
    int zero_size = 1024*2*channel_;
    char* zero_pcm = new char[zero_size];
    memset(zero_pcm, 0, zero_size);

    int audio_frame_count = 0;

    for (int i = 0; i < sample_count; ++i)
    {
        long long frame_pts = av_rescale_q(audio_frame_count,
            av_make_q(1, samrate_), av_make_q(1, 1000));

        audio_frame_count += 1024;

        if (isOpening)
        {
            if (mp4_writter_)
                mp4_writter_->WriteAudioOpeningData(zero_pcm, zero_size, frame_pts);
            if (flv_writter_)
                flv_writter_->WriteAudioOpeningData(zero_pcm, zero_size, frame_pts);
        }
        else
        {
            if (mp4_writter_)
                mp4_writter_->WriteAudioEndingData(zero_pcm, zero_size, frame_pts);
            if (flv_writter_)
                flv_writter_->WriteAudioEndingData(zero_pcm, zero_size, frame_pts);
        }
    }

    delete[] zero_pcm;
}

/*******************************************************************************
 * avi_muxer.cpp
 * Copyright: (c) 2014 Haibin Du(haibindev.cnblogs.com). All rights reserved.
 * -----------------------------------------------------------------------------
 *
 * -----------------------------------------------------------------------------
 * 2015-2-13 16:54 - Created (Haibin Du)
 ******************************************************************************/

#include "avi_muxer.h"

#include <mmreg.h>

#include "base/simple_logger.h"
#include "record/FFEncoder.h"
#include "record/FFWritter.h"

//#define AUDIO_USE_ENCODER 1

AviMuxer::AviMuxer(const std::string& filename)
{
    //remove(filename.c_str());
    avi_writter_ = new FFWritter(filename, "avi");
    width_ = 0;
    height_ = 0;
    fps_ = 0;
    bitrate_ = 0;
    samrate_ = 0;
    channel_ = 0;

    audio_encoder_ = NULL;

    video_encoder_ = NULL;

    avi_name_ = filename;

    opening_millsecs_ = 0;
    ending_millsecs_ = 0;
}

AviMuxer::~AviMuxer()
{
    Close();
}

void AviMuxer::SetOpeningFile(const std::string& filename,
    int opendingMillsecs)
{
    opening_name_ = filename;
    opening_millsecs_ = opendingMillsecs;
}

void AviMuxer::SetEndingFile(const std::string& filename,
    int endingMillsecs)
{
    ending_name_ = filename;
    ending_millsecs_ = endingMillsecs;
}

void AviMuxer::Open(int width, int height, int fps, int bitrate,
    int samrate, int channel)
{
    width_ = width;
    height_ = height;
    fps_ = fps;
    bitrate_ = bitrate;
    samrate_ = samrate;
    channel_ = channel;

#if defined(AUDIO_USE_ENCODER)
    AVCodecID audio_codec_id = AV_CODEC_ID_MP2;
#else
    AVCodecID audio_codec_id = AV_CODEC_ID_PCM_S16LE;
#endif

    // 首先创建AVI格式文件
    if (avi_writter_)
    {
        avi_writter_->Open(true, AV_CODEC_ID_MPEG4,
            width, height, fps, bitrate,
            true, audio_codec_id, samrate, channel);
    }

#if defined(AUDIO_USE_ENCODER)
    audio_encoder_ = new FFAudioEncoder(audio_codec_id);
    audio_encoder_->Init(samrate, channel, 16000);
    audio_buf_.resize(audio_encoder_->BufSize());
#endif

    // 创建MJPEG编码器
    video_encoder_ = new FFVideoEncoder(AV_CODEC_ID_MPEG4);
    video_encoder_->Init(width, height, fps, bitrate);

    mjpeg_buf_.resize(width*height*4);

    if (false == opening_name_.empty())
        AppendOpening();

    time_counter_.Reset();
}

void AviMuxer::Close()
{
    if (!avi_writter_) return;

    if (false == ending_name_.empty())
        AppendEnding();

    avi_writter_->Close();
    delete avi_writter_;
    avi_writter_ = NULL;

    delete audio_encoder_;
    audio_encoder_ = NULL;

    delete video_encoder_;
    video_encoder_ = NULL;
}

void AviMuxer::SetPause(bool isPause)
{
    if (avi_writter_)
    {
        avi_writter_->SetPause(isPause);
    }
}

void AviMuxer::OnPcmData(const char* dataBuf, int dataSize)
{
    if (audio_encoder_)
    {
        // 这部分代码无用，不需要音频编码
        int outlen = audio_buf_.size();
        audio_encoder_->Encode((unsigned char*)dataBuf, dataSize, &audio_buf_[0], outlen);
        if (outlen > 0 && avi_writter_)
        {
            avi_writter_->WriteAudioEncodedData((char*)&audio_buf_[0], outlen);
        }
    }
    else if (avi_writter_)
    {
        // 直接PCM写入到AVI格式中
        avi_writter_->WriteAudioEncodedData(dataBuf, dataSize);
    }
}

void AviMuxer::OnRgbData(const char* dataBuf, int dataSize, long long shouldTS)
{
    if (video_encoder_)
    {
        int outlen = mjpeg_buf_.size();
        bool iskey = false;
        video_encoder_->Encode(dataBuf, dataSize, &mjpeg_buf_[0], outlen, iskey);
        if (outlen > 0 && avi_writter_)
        {
            avi_writter_->WriteVideoEncodedData(&mjpeg_buf_[0], outlen, iskey, shouldTS*1000);
        }
    }
}

void AviMuxer::OnYuvData(const char* dataBuf, int dataSize, long long shouldTS)
{
    if (video_encoder_)
    {
        int outlen = mjpeg_buf_.size();
        bool iskey = false;
        video_encoder_->EncodeYuv(dataBuf, dataSize, &mjpeg_buf_[0], outlen, iskey);
        if (outlen > 0 && avi_writter_)
        {
            avi_writter_->WriteVideoEncodedData(&mjpeg_buf_[0], outlen, iskey, shouldTS);
        }
    }
}

void AviMuxer::AppendOpening()
{
    SIMPLE_LOG("\n");

    Appending(true);
}

void AviMuxer::AppendEnding()
{
    SIMPLE_LOG("\n");

    Appending(false);
}

void AviMuxer::Appending(bool isOpening)
{
    std::string appending_name = isOpening ? opening_name_ : ending_name_;

    SIMPLE_LOG("isopen: %d, name: %s\n", isOpening, appending_name.c_str());

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

void AviMuxer::AppendingVideo(FFReader& ffReader, bool isOpening)
{
    std::string appending_name = isOpening ? opening_name_ : ending_name_;

    if (isOpening)
    {
        if (ffReader.AudioCodecId() == AV_CODEC_ID_NONE)    // 没有音频
        {
            avi_writter_->SetOpeningDuration(ffReader.VideoDuration(), ffReader.VideoDuration());
        }
        else
        {
            avi_writter_->SetOpeningDuration(ffReader.AudioDuration(), ffReader.VideoDuration());
        }
    }

    int data_size = 0;
    int data_type = -1;
    long long data_time = 0;
    for (;;)
    {
        char* data_buf = ffReader.ReadFrame(&data_type, &data_size, &data_time);
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
                avi_writter_->WriteAudioOpeningData(data_buf, data_size, data_time);
            else
                avi_writter_->WriteAudioEndingData(data_buf, data_size, data_time);
        }
        else if (data_type == 2)    // 视频
        {
            if (isOpening)
                avi_writter_->WriteVideoOpeningData(data_buf, data_size, data_time);
            else
                avi_writter_->WriteVideoEndingData(data_buf, data_size, data_time);
        }

        ffReader.FreeFrame();
    }

    if (ffReader.AudioCodecId() == AV_CODEC_ID_NONE)
    {
        // 附加空音频
        AppendingZeroPCM(ffReader.VideoDuration(), isOpening);
    }
}

void AviMuxer::AppendingPicture(FFReader& ffReader, bool isOpening)
{
    if (isOpening)
    {
        avi_writter_->SetOpeningDuration(opening_millsecs_, opening_millsecs_);
    }

    int data_size = 0;
    int data_type = -1;
    long long data_time = 0;
    char* data_buf = ffReader.ReadFrame(&data_type, &data_size, &data_time);
//     if (data_type < 0 || data_time > ffReader.AudioDuration())
//     {
//         return;
//     }

    // 按帧率写入重复图片
    int repeat_count = opening_millsecs_ * fps_ / 1000;
    for (int i = 0; i < repeat_count; ++i)
    {
        long long timestamp = i * 1000 / fps_;

        if (isOpening)
            avi_writter_->WriteVideoOpeningData(data_buf, data_size, timestamp);
        else
            avi_writter_->WriteVideoEndingData(data_buf, data_size, timestamp);
    }

    ffReader.FreeFrame();

    // 附加空音频
    AppendingZeroPCM(opening_millsecs_, isOpening);
}

void AviMuxer::AppendingZeroPCM(long long pcmMillsecs, bool isOpening)
{
    int sample_count = (long long)pcmMillsecs*samrate_/1024000;
    int zero_size = 1024*2*channel_;
    char* zero_pcm = new char[zero_size];
    memset(zero_pcm, 0, zero_size);

    for (int i = 0; i < sample_count; ++i)
    {
        if (isOpening)
            avi_writter_->WriteAudioOpeningData(zero_pcm, zero_size, -1);
        else
            avi_writter_->WriteAudioEndingData(zero_pcm, zero_size, -1);
    }

    delete[] zero_pcm;
}

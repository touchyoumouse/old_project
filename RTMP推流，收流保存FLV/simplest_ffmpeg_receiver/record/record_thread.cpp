/*******************************************************************************
 * record_thread.cpp
 * Copyright: (c) 2014 Haibin Du(haibindev.cnblogs.com). All rights reserved.
 * -----------------------------------------------------------------------------
 *
 * -----------------------------------------------------------------------------
 * 2016-2-25 21:45 - Created (Haibin Du)
 ******************************************************************************/

#include "record_thread.h"

#include <limits>

#include "base/simple_logger.h"
#include "base_encoder/yuv_convert.h"

#include "record/avi_muxer.h"
#include "record/recoder.h"

RecordThread::RecordThread(const std::string& savePath,
    bool isSaveAvi, bool isSaveMp4, bool isSaveFlv)
    : save_path_(savePath)
{
    std::string::size_type pos = save_path_.rfind(".");
    if (pos == save_path_.size()-4 && (
        save_path_[save_path_.size()-3] == 'a' || save_path_[save_path_.size()-3] == 'A' ||
        save_path_[save_path_.size()-3] == 'f' || save_path_[save_path_.size()-3] == 'F' ||
        save_path_[save_path_.size()-3] == 'm' || save_path_[save_path_.size()-3] == 'M')
        )
    {
        save_path_ = save_path_.substr(0, pos);
    }

    is_save_avi_ = isSaveAvi;
    is_save_mp4_ = isSaveMp4;
    is_save_flv_ = isSaveFlv;

    opening_millsecs_ = -1;
    ending_millsecs_ = -1;

    is_has_audio_ = true;
    samplerate_ = 0;
    channle_ = 0;
    audio_bitrate_ = 0;

    width_ = 0;
    height_ = 0;
    fps_ = 10;
    video_bitrate_ = 0;
    bitrate_mp4_ = 0;
    quality_ = 0;
    is_mp4_use_cbr_ = false;

    recorder_ = NULL;
    aac_extra_data_ = NULL;
    aac_extra_size_ = 0;

    avi_muxer_ = NULL;

    is_recording_ = false;
    is_pause_ = false;
    is_stop_ = false;
}

RecordThread::~RecordThread()
{
    delete[] aac_extra_data_;
}

void RecordThread::SetOpeningEnding(
    const std::string& openAviName, const std::string& openMp4Name,
    const std::string& openFlvName, int openingMillsecs,
    const std::string& endAviName, const std::string& endMp4Name,
    const std::string& endFlvName, int endingMillsecs)
{
    opening_avi_ = openAviName;
    opening_mp4_ = openMp4Name;
    opening_flv_ = openFlvName;
    opening_millsecs_ = openingMillsecs;
    ending_avi_ = endAviName;
    ending_mp4_ = endMp4Name;
    ending_flv_ = endFlvName;
    ending_millsecs_ = endingMillsecs;
}

void RecordThread::Start(bool isHasAudio, int sampleRate, int channel, int audioBitrate,
    int width, int height, int fps, int videoBitrate, int bitrateMp4,
    int quality, bool isMp4UseCBR)
{
    base::AutoLock al(write_mtx_);

    // 防止函数重复被调用
    if (width_ > 0 || samplerate_ > 0)
    {
        return;
    }

    is_has_audio_ = isHasAudio;
    samplerate_ = sampleRate;
    channle_ = channel;
    audio_bitrate_ = audioBitrate;

    width_ = width;
    height_ = height;
    fps_ = fps;
    video_bitrate_ = videoBitrate;
    bitrate_mp4_ = bitrateMp4;
    quality_ = quality;
    is_mp4_use_cbr_ = isMp4UseCBR;

    is_pause_ = false;

    ThreadStart();
}

void RecordThread::Stop()
{
    if (is_stop_) return;

    is_stop_ = true;
    is_recording_ = false;

    SIMPLE_LOG("publisher joinning\n");

    ThreadStop();
    ThreadJoin();

    SIMPLE_LOG("join end\n");

    base::AutoLock al(write_mtx_);

    SIMPLE_LOG("try close encoder\n");


    SIMPLE_LOG("try close recorder_\n");

    if (recorder_)
    {
        recorder_->Stop();
        delete recorder_;
        recorder_ = NULL;
    }

    SIMPLE_LOG("try close muxer\n");

    // 关闭文件
    if (avi_muxer_)
    {
        avi_muxer_->Close();
        delete avi_muxer_;
        avi_muxer_ = NULL;
    }

    SIMPLE_LOG("end\n");
}

void RecordThread::Pause()
{
    base::AutoLock al(write_mtx_);

    is_pause_ = !is_pause_;

    if (recorder_)
    {
        recorder_->Pause();
    }

    if (avi_muxer_)
    {
        avi_muxer_->SetPause(is_pause_);
    }
}

void RecordThread::CreateRecordFile()
{
    // 检查并创建avi文件
    if (is_save_avi_)
    {
        SIMPLE_LOG("creating avi, open: %s, end: %s\n", opening_avi_.c_str(), ending_avi_.c_str());

        avi_muxer_ = new AviMuxer(save_path_+".avi");
        if (false == opening_avi_.empty())
            avi_muxer_->SetOpeningFile(opening_avi_, opening_millsecs_);
        if (false == ending_avi_.empty())
            avi_muxer_->SetEndingFile(ending_avi_, ending_millsecs_);
        avi_muxer_->Open(width_, height_, fps_, video_bitrate_, samplerate_, channle_);
    }

    if (is_save_mp4_ || is_save_flv_)
    {
        SIMPLE_LOG("creating mp4 or flv, open: %s, end: %s\n", opening_mp4_.c_str(), ending_mp4_.c_str());

        int save_type = 0;
        if (is_save_mp4_) save_type |= Recorder::SAVE_FILE_MP4;
        if (is_save_flv_) save_type |= Recorder::SAVE_FILE_FLV;

        recorder_ = new Recorder((Recorder::SaveFileType)save_type, save_path_);
        if (false == opening_mp4_.empty())
            recorder_->SetOpeningFile(opening_mp4_, opening_millsecs_);
        if (false == ending_mp4_.empty())
            recorder_->SetEndingFile(ending_mp4_, ending_millsecs_);
        recorder_->Start(true, samplerate_, channle_,
            true, width_, height_, fps_, bitrate_mp4_);

        if (aac_extra_data_ && aac_extra_size_)
        {
            recorder_->OnAACExtra(aac_extra_data_, aac_extra_size_);
        }
    }

    SIMPLE_LOG("end\n");
}
//@feng 创建文件文件并保存片头
void RecordThread::ThreadRun()
{
//    SIMPLE_LOG("interval: %lld\n", interval);

    CreateRecordFile();

	
    is_recording_ = true;

    while (false == IsThreadStop())
    {
        long long interval = 1000.0/fps_;
        TimeCounter timer;
        timer.Reset();

        long long time_diff = interval - timer.Get();
        if (time_diff > 0)
        {
            MillsecSleep(time_diff);
        }
    }

    SIMPLE_LOG("\n");
}

void RecordThread::OnPcmBufferToAvi(const char* pcmBuf, int bufSize)
{
    if (is_stop_ || !is_recording_) return;

    base::AutoLock al(write_mtx_);

    if (is_pause_) return;

    if (avi_muxer_)
    {
        avi_muxer_->OnPcmData(pcmBuf, bufSize);
    }
}

void RecordThread::OnRgbBufferToAvi(const char* rgbBuf, int bufSize, long long shouldTS)
{
    if (is_stop_) return;

    if (is_pause_) return;

    // AviMuxer可以直接处理RGB
    if (avi_muxer_)
    {
        avi_muxer_->OnRgbData(rgbBuf, bufSize, shouldTS);
    }

    return;
}

void RecordThread::OnH264Extra(const char* extraBuf, int extraSize)
{
    if (is_stop_ || !is_recording_) return;
}

void RecordThread::OnH264Frame(const char* frameBuf, int frameSize,
    bool isKeyframe, long long shouldTS)
{
    if (is_stop_ || !is_recording_) return;

    if (recorder_)
    {
        recorder_->OnH264Frame(frameBuf, frameSize, isKeyframe, shouldTS);
    }
}

void RecordThread::OnAACExtra(const char* extraBuf, int extraSize)
{
    if (is_stop_) return;

    if (!is_recording_)
    {
        delete[] aac_extra_data_;
        aac_extra_data_ = new char[extraSize];
        memcpy(aac_extra_data_, extraBuf, extraSize);
        aac_extra_size_ = extraSize;

        return;
    }

    if (recorder_)
    {
        recorder_->OnAACExtra(extraBuf, extraSize);
    }
}

void RecordThread::OnAACFrame(const char* frameBuf, int frameSize)
{
    if (is_stop_ || !is_recording_) return;

    if (recorder_)
    {
        recorder_->OnAACFrame(frameBuf, frameSize);
    }
}

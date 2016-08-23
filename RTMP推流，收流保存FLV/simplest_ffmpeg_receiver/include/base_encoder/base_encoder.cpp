/*******************************************************************************
 * base_encoder.cpp
 * Copyright: (c) 2013 Haibin Du(haibinnet@qq.com). All rights reserved.
 * -----------------------------------------------------------------------------
 *
 * -----------------------------------------------------------------------------
 * 2014-4-20 14:41 - Created (Haibin Du)
 ******************************************************************************/

#include "base_encoder.h"

#include "base/simple_logger.h"

BaseEncoder::BaseEncoder()
{
    observer_ = NULL;

    yuv_convert_ = NULL;
    x264_encoder_ = NULL;
    live_264size_ = 0;
    live_264buf_ = NULL;

    has_get_sps_pps_ = false;

    faac_encoder_ = NULL;
    faac_size_ = 0;
    faac_buf_ = NULL;

    width_ = 0;
    height_ = 0;
    fps_ = 0;
    is_need_reverse_ = true;

    video_buffer_ = NULL;
    is_data_update_ = false;

    is_need_keyframe_rightnow_ = false;
}

BaseEncoder::~BaseEncoder()
{
    Stop();
}

void BaseEncoder::SetObserver(Observer* ob)
{
    observer_ = ob;
}

void BaseEncoder::Start()
{
    ThreadStart();
}

void BaseEncoder::Stop()
{
    if (IsThreadStop()) return;

    ThreadStop();
    ThreadJoin();

    delete video_buffer_;
    
    delete[] live_264buf_;
    
    delete x264_encoder_;
    delete yuv_convert_;

    delete unit_helper_;
    delete[] faac_buf_;
    delete faac_encoder_;
}

void BaseEncoder::SetNeedKeyframe()
{
    is_need_keyframe_rightnow_ = true;
}

void BaseEncoder::ThreadRun()
{
    timer_.Reset();

    RawBuffer* raw_buffer = NULL;

    while (true)
    {
        if (base::SimpleThread::IsThreadStop()) break;

        long long next_tick = timer_.Now() + 1000.0/fps_;

        {
            base::AutoLock al(buffer_lock_);
            if (video_buffer_ && is_data_update_)
            {
                if (raw_buffer == NULL)
                {
                    raw_buffer = new RawBuffer(video_buffer_->data_buf,
                        video_buffer_->data_size, video_buffer_->raw_type);
                }
                else
                {
                    memcpy(raw_buffer->data_buf, video_buffer_->data_buf,
                        video_buffer_->data_size);
                }
                is_data_update_ = false;
            }
        }

        if (raw_buffer)
        {
            switch (raw_buffer->raw_type)
            {
            case RAW_TYPE_RGB24:
                OnRGB24Buffer(raw_buffer->data_buf, raw_buffer->data_size);
                break;
            case RAW_TYPE_RGB32:
                OnRGB32Buffer(raw_buffer->data_buf, raw_buffer->data_size);
                break;
            case RAW_TYPE_YUY2:
                OnYUY2Buffer(raw_buffer->data_buf, raw_buffer->data_size);
                break;
            case RAW_TYPE_YUV420P:
                OnYuv420PData(raw_buffer->data_buf, raw_buffer->data_size);
                break;
            }
        }

        long long now_tick = timer_.Now();
        if (next_tick > now_tick)
        {
            MillsecSleep(static_cast<int>(next_tick - now_tick));
        }
    } // while

    delete raw_buffer;
}

void BaseEncoder::InitVideoEncodeParam(int width, int height, int bitrate,
    int fps, int keyInter, int quality, bool useCBR, bool isReverse)
{
    width_ = width;
    height_ = height;
    fps_ = fps;
    is_need_reverse_ = isReverse;

    SIMPLE_LOG("\n");

    x264_encoder_ = new X264Encoder();
    x264_encoder_->Init(width_, height_, bitrate, fps_, quality, keyInter, useCBR);

    live_264size_ = width*height_*4;
    live_264buf_ = new char[live_264size_];

    SIMPLE_LOG("\n");

    unsigned char x264codecinfo[2048];
    int info_size = 0;
    x264_encoder_->GetExtraCodecInfo(x264codecinfo, &info_size);
    if (info_size > 0)
    {
        SIMPLE_LOG("\n");
        observer_->OnH264Extra((char*)x264codecinfo, info_size);
    }
    SIMPLE_LOG("end\n");
}

void BaseEncoder::InitAudioEncodeParam(int samplerate, int channel,
    int bitrate, bool isNeedAdts)
{
    SIMPLE_LOG("\n");

    unit_helper_ = new BufUnitHelper(1024*channel*2);

    faac_encoder_ = new FAACEncoder();
    faac_encoder_->Init(isNeedAdts, samplerate, channel, 16, bitrate);

    SIMPLE_LOG("\n");

    faac_size_ = faac_encoder_->MaxOutBytes();
    faac_buf_ = new char[faac_size_];

    unsigned char* decodeinfo = NULL;
    int infosize = 0;
    faac_encoder_->GetDecodeInfoBuf(&decodeinfo, &infosize);
    if (infosize > 0)
    {
        SIMPLE_LOG("\n");
        observer_->OnAACExtra((char*)decodeinfo, infosize);
    }
    SIMPLE_LOG("end\n");
}

// Êý¾Ý

void BaseEncoder::PostRGB24Buffer(const char* dataBuf, int dataSize)
{
    if (IsThreadStop()) return;

    base::AutoLock al(buffer_lock_);

    if (NULL == video_buffer_)
    {
        video_buffer_ = new RawBuffer(dataBuf, dataSize, RAW_TYPE_RGB24);
    }
    else
    {
        memcpy(video_buffer_->data_buf, dataBuf, dataSize);
        video_buffer_->raw_type = RAW_TYPE_RGB24;
    }

    is_data_update_ = true;
}

void BaseEncoder::PostRGB32Buffer(const char* dataBuf, int dataSize)
{
    if (IsThreadStop()) return;

    base::AutoLock al(buffer_lock_);

    if (NULL == video_buffer_)
    {
        video_buffer_ = new RawBuffer(dataBuf, dataSize, RAW_TYPE_RGB32);
    }
    else
    {
        memcpy(video_buffer_->data_buf, dataBuf, dataSize);
        video_buffer_->raw_type = RAW_TYPE_RGB32;
    }

    is_data_update_ = true;
}

void BaseEncoder::PostYUY2Buffer(const char* dataBuf, int dataSize)
{
    if (IsThreadStop()) return;

    base::AutoLock al(buffer_lock_);

    if (NULL == video_buffer_)
    {
        video_buffer_ = new RawBuffer(dataBuf, dataSize, RAW_TYPE_YUY2);
    }
    else
    {
        memcpy(video_buffer_->data_buf, dataBuf, dataSize);
        video_buffer_->raw_type = RAW_TYPE_YUY2;
    }

    is_data_update_ = true;
}

void BaseEncoder::PostYuv420Buffer(const char* dataBuf, int dataSize)
{
    if (IsThreadStop()) return;

    base::AutoLock al(buffer_lock_);

    if (NULL == video_buffer_)
    {
        video_buffer_ = new RawBuffer(dataBuf, dataSize, RAW_TYPE_YUV420P);
    }
    else
    {
        memcpy(video_buffer_->data_buf, dataBuf, dataSize);
        video_buffer_->raw_type = RAW_TYPE_YUV420P;
    }

    is_data_update_ = true;
}

void BaseEncoder::OnPCMBuffer(const char* dataBuf, int dataSize)
{
    if (IsThreadStop()) return;

//     if (NULL == video_buffer_)
//     {
//         return;
//     }

    if (dataSize == unit_helper_->UnitSize())
    {
        int sample_count = (dataSize >> 1);
        int outlen = 0;
        faac_encoder_->Encode((unsigned char*)dataBuf, sample_count,
            (unsigned char*)faac_buf_, outlen);

        if (outlen > 0 && observer_)
        {
            observer_->OnAACFrame(faac_buf_, outlen);
        }
    }
    else
    {
        unit_helper_->PushBuf((char*)dataBuf, dataSize);
        while (unit_helper_->ReadBuf())
        {
            int sample_count = (unit_helper_->UnitSize() >> 1);
            int outlen = 0;
            faac_encoder_->Encode((unsigned char*)unit_helper_->UnitBuf(), sample_count,
                (unsigned char*)faac_buf_, outlen);

            if (outlen > 0 && observer_)
            {
                observer_->OnAACFrame(faac_buf_, outlen);
            }
        }
    }
}

void BaseEncoder::OnRGB24Buffer(const char* dataBuf, int dataSize)
{
    if (IsThreadStop()) return;

    CheckYuvConvert(RAW_TYPE_RGB24);

    yuv_convert_->ConvertFromRGB24(dataBuf, dataSize);

    OnYuv420PData(yuv_convert_->YuvBuf(), yuv_convert_->YuvSize());
}

void BaseEncoder::OnRGB32Buffer(const char* dataBuf, int dataSize)
{
    if (IsThreadStop()) return;

    CheckYuvConvert(RAW_TYPE_RGB32);

    yuv_convert_->ConvertFromRGB32(dataBuf, dataSize);

    OnYuv420PData(yuv_convert_->YuvBuf(), yuv_convert_->YuvSize());
}

void BaseEncoder::OnYUY2Buffer(const char* dataBuf, int dataSize)
{
    if (IsThreadStop()) return;

    CheckYuvConvert(RAW_TYPE_YUY2);

    yuv_convert_->ConvertFromYUY2(dataBuf, dataSize);

    OnYuv420PData(yuv_convert_->YuvBuf(), yuv_convert_->YuvSize());
}

void BaseEncoder::OnYuv420PData(const char* buf, int bufLen)
{
    int outlen = live_264size_;
    bool is_keyframe = false;

    if (is_need_keyframe_rightnow_)
    {
        is_keyframe = true;
        is_need_keyframe_rightnow_ = false;
    }

    char* nalbuf = x264_encoder_->Encode((unsigned char*)buf, 
        (unsigned char*)live_264buf_, outlen, is_keyframe);

    if (outlen > 0 && observer_)
    {
        observer_->OnH264Frame(nalbuf, outlen, is_keyframe);
    }
}

void BaseEncoder::CheckYuvConvert(RawType pixFmt)
{
    if (yuv_convert_) return;

    if (yuv_convert_ == NULL)
    {
        yuv_convert_ = new YuvConvert(width_, height_);

        if (pixFmt == RAW_TYPE_RGB24)
        {
            yuv_convert_->InitFromRGB24(is_need_reverse_);
        }
        else if (pixFmt == RAW_TYPE_RGB32)
        {
            yuv_convert_->InitFromRGB32(is_need_reverse_);
        }
        else if (pixFmt == RAW_TYPE_YUY2)
        {
            yuv_convert_->InitFromYUY2(is_need_reverse_);
        }
    }
}

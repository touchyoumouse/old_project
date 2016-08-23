/*******************************************************************************
 * base_encoder.h
 * Copyright: (c) 2013 Haibin Du(haibinnet@qq.com). All rights reserved.
 * -----------------------------------------------------------------------------
 *
 * 封装h.264编码和aac编码，传入原始数据，即可获得编码后数据。包含了音频sample缓冲器，可以
 * 不用考虑传入的pcm数据长度是否符合aac标准，会自行调整。
 *
 * -----------------------------------------------------------------------------
 * 2014-4-20 14:41 - Created (Haibin Du)
 ******************************************************************************/

#ifndef _HBASE_BASE_ENCODER_H_
#define _HBASE_BASE_ENCODER_H_

#include <deque>
#include <string>

#include "base/Lock.h"
#include "base/SimpleThread.h"
#include "base/TimeCounter.h"
#include "base/buf_unit_helper.h"

#include "yuv_convert.h"
#include "x264_encoder.h"
#include "faac_encoder.h"

class BaseEncoder :public base::SimpleThread
{
public:
    class Observer
    {
    public:
        virtual ~Observer() {}

        virtual void OnH264Extra(const char* extraBuf, int extraSize) = 0;
        virtual void OnH264Frame(const char* frameBuf, int frameSize,
            bool isKeyframe) = 0;
        virtual void OnAACExtra(const char* extraBuf, int extraSize) = 0;
        virtual void OnAACFrame(const char* frameBuf, int frameSize) = 0;
    };

public:
    enum RawType
    {
        RAW_TYPE_RGB24,
        RAW_TYPE_RGB32,
        RAW_TYPE_YUV420P,
        RAW_TYPE_YUY2,

        RAW_TYPE_PCM,
    };

    struct RawBuffer
    {        
        char* data_buf;
        int data_size;
        RawType raw_type;

        RawBuffer(const char* srcData, int srcSize, RawType srcType)
        {
            data_buf = new char[srcSize];
            memcpy(data_buf, srcData, srcSize);
            data_size = srcSize;
            raw_type = srcType;
        }

        ~RawBuffer()
        {
            delete[] data_buf;
        }
    };

public:
    BaseEncoder();

    ~BaseEncoder();

    void SetObserver(Observer* ob);

    void InitVideoEncodeParam(int width, int height, int bitrate, 
        int fps, int keyInter, int quality, bool useCBR, bool isReverse);

    void InitAudioEncodeParam(int samplerate, int channel, int bitrate,
        bool isNeedAdts);

    void Start();

    void Stop();

    void SetNeedKeyframe();

    virtual void ThreadRun();

    // 数据放在线程中编码回调
    void PostRGB24Buffer(const char* dataBuf, int dataSize);

    void PostRGB32Buffer(const char* dataBuf, int dataSize);

    void PostYUY2Buffer(const char* dataBuf, int dataSize);

    void PostYuv420Buffer(const char* dataBuf, int dataSize);

    // 数据立刻编码回调
    void OnPCMBuffer(const char* dataBuf, int dataSize);

    void OnRGB24Buffer(const char* dataBuf, int dataSize);

    void OnRGB32Buffer(const char* dataBuf, int dataSize);

    void OnYUY2Buffer(const char* dataBuf, int dataSize);

    void OnYuv420PData(const char* buf, int bufLen);

private:
    void CheckYuvConvert(RawType pixFmt);

private:
    Observer* observer_;
    TimeCounter timer_;

    // 编码器
    YuvConvert* yuv_convert_;

    X264Encoder* x264_encoder_;
    char* live_264buf_;
    int live_264size_;

    bool has_get_sps_pps_;

    FAACEncoder* faac_encoder_;
    char* faac_buf_;
    int faac_size_;

    BufUnitHelper* unit_helper_;

    int width_;
    int height_;
    int fps_;
    bool is_need_reverse_;

    bool is_data_update_;
    RawBuffer* video_buffer_;
    base::Lock buffer_lock_;

    volatile bool is_need_keyframe_rightnow_;
};

#endif // _HBASE_BASE_ENCODER_H_

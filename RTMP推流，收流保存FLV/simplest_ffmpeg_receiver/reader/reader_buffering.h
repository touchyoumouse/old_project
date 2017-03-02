/*******************************************************************************
 * reader_buffering.h
 * Copyright: (c) 2014 Haibin Du(haibindev.cnblogs.com). All rights reserved.
 * -----------------------------------------------------------------------------
 *
 * ����Դ���ݻ�����С�
 * ���������Դ��ȡ������Ƶ���ݣ���Ƶ���ݣ�����ʱ������лص���
 *
 * -----------------------------------------------------------------------------
 * 2016-7-28 10:47 - Created (Haibin Du)
 ******************************************************************************/

#ifndef _HDEV_READER_BUFFERING_H_
#define _HDEV_READER_BUFFERING_H_

#include "../base/base.h"

#include <deque>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavutil/opt.h"
}

#include "../base/Lock.h"
#include "../base/SimpleThread.h"
#include "../base/TimeCounter.h"

//////////////////////////////////////////////////////////////////////////
// ����ṹ��

class BufferingCache
{
public:
    struct CacheBuf 
    {
        char* buf_;
        int buflen_;
        int capacity_;
        long long timestamp_;

        CacheBuf(int initSize);

        ~CacheBuf();

        void CheckCapacity(int newSize);

        void Assign(const char* srcBuf, int srcBuflen, long long ts);

        void Clear();
    };

public:
    BufferingCache();

    ~BufferingCache();

    CacheBuf* CacheMalloc(int needSize);

    void CacheFree(CacheBuf* cache);

private:
    std::deque<CacheBuf*> cache_bufs_;
};

//////////////////////////////////////////////////////////////////////////
// ��Ƶ�������

class AudioBufferingQueue : base::SimpleThread
{
public:
    class Observer
    {
    public:
        virtual ~Observer() {}

        virtual void OnAudioQueueData(const char* dataBuf, int dataSize) = 0;
    };

public:
    AudioBufferingQueue(Observer* ob, int samRate);

    ~AudioBufferingQueue();

    void Start();

    void Stop();

    void Clear();

    void Post(const char* dataBuf, int dataSize, long long timestamp);

    // ------------------------------------------------------------
    // ʵ�� hbase::DelegateSimpleThread::Delegate
    virtual void ThreadRun();

private:
    volatile bool is_stopping_;

    Observer* observer_;
    std::deque<BufferingCache::CacheBuf*> buf_queue_;
    base::Lock queue_mtx_;

    TimeCounter time_counter_;
    long long base_timestamp_;
    int sample_rate_;
    long long audio_frame_count_;

    BufferingCache cache_;
};

//////////////////////////////////////////////////////////////////////////
// ��Ƶ�������

class VideoBufferingQueue : base::SimpleThread
{
public:
    class Observer
    {
    public:
        virtual ~Observer() {}

        virtual void OnVideoQueueData(const char* dataBuf, int dataSize,
            long long timestamp) = 0;
    };

public:
    VideoBufferingQueue(Observer* ob);

    ~VideoBufferingQueue();

    void Start();

    void Stop();

    void Clear();

    void Post(const char* dataBuf, int dataSize, long long timestamp);

    // ------------------------------------------------------------
    // ʵ�� hbase::DelegateSimpleThread::Delegate
    virtual void ThreadRun();

private:
    volatile bool is_stopping_;

    Observer* observer_;
    std::deque<BufferingCache::CacheBuf*> buf_queue_;
    base::Lock queue_mtx_;
    TimeCounter time_counter_;
    long long base_timestamp_;

    BufferingCache cache_;
};

#endif // _HDEV_READER_BUFFERING_H_

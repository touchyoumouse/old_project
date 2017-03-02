/*******************************************************************************
 * reader_buffering.cpp
 * Copyright: (c) 2014 Haibin Du(haibindev.cnblogs.com). All rights reserved.
 * -----------------------------------------------------------------------------
 *
 * -----------------------------------------------------------------------------
 * 2016-7-28 10:56 - Created (Haibin Du)
 ******************************************************************************/

#include "reader_buffering.h"

extern "C"
{
#include "libavformat/avformat.h"
}

#include "../base/simple_logger.h"

#define CACHE_DELAY_MILLSECS 400

//////////////////////////////////////////////////////////////////////////
// BufferingCache

BufferingCache::CacheBuf::CacheBuf(int initSize)
{
    buf_ = new char[initSize];
    buflen_ = 0;
    capacity_ = initSize;
    timestamp_ = 0;
}

BufferingCache::CacheBuf::~CacheBuf()
{
    delete[] buf_;
}

void BufferingCache::CacheBuf::CheckCapacity(int newSize)
{
    if (newSize > capacity_)
    {
        delete[] buf_;
        buf_ = new char[newSize];
        buflen_ = 0;
        capacity_ = newSize;
    }
}

void BufferingCache::CacheBuf::Assign(const char* srcBuf, int srcBuflen,
    long long ts)
{
    memcpy(buf_, srcBuf, srcBuflen);
    buflen_ = srcBuflen;
    timestamp_ = ts;
}

void BufferingCache::CacheBuf::Clear()
{
    buflen_ = 0;
}

// BufferingCache

BufferingCache::BufferingCache()
{

}

BufferingCache::~BufferingCache()
{
    while (false == cache_bufs_.empty())
    {
        CacheBuf* buf = cache_bufs_.front();
        cache_bufs_.pop_front();

        delete buf;
    }
}

BufferingCache::CacheBuf* BufferingCache::CacheMalloc(int needSize)
{
    CacheBuf* buf = NULL;

    if (cache_bufs_.empty())
    {
        buf = new CacheBuf(needSize);
    }
    else
    {
        buf = cache_bufs_.front();
        cache_bufs_.pop_front();
        buf->CheckCapacity(needSize);
    }

    return buf;
}

void BufferingCache::CacheFree(CacheBuf* cache)
{
    cache->Clear();
    cache_bufs_.push_back(cache);
}

//////////////////////////////////////////////////////////////////////////
// AudioBufferingQueue

AudioBufferingQueue::AudioBufferingQueue(Observer* ob, int samRate)
{
    is_stopping_ = false;

    observer_ = ob;
    base_timestamp_ = -1;
    sample_rate_ = samRate;
    audio_frame_count_ = 0;
}

AudioBufferingQueue::~AudioBufferingQueue()
{

}

void AudioBufferingQueue::Start()
{
    ThreadStart();
}

void AudioBufferingQueue::Stop()
{
    if (is_stopping_) return;

    is_stopping_ = true;

    ThreadStop();
    ThreadJoin();

    while (false == buf_queue_.empty())
    {
        BufferingCache::CacheBuf* buf = buf_queue_.front();
        buf_queue_.pop_front();

        cache_.CacheFree(buf);
    }
}

void AudioBufferingQueue::Clear()
{
    base::AutoLock lg(queue_mtx_);

    base_timestamp_ = -1;
    audio_frame_count_ = 0;

    while (false == buf_queue_.empty())
    {
        BufferingCache::CacheBuf* buf = buf_queue_.front();
        buf_queue_.pop_front();

        cache_.CacheFree(buf);
    }
}

void AudioBufferingQueue::Post(const char* dataBuf, int dataSize,
    long long timestamp)
{
    if (is_stopping_) return;

    if (time_counter_.LastTimestamp() == -1)
    {
        time_counter_.Reset();
    }

    if (base_timestamp_ == -1)
    {
        base_timestamp_ = timestamp;
    }

    base::AutoLock lg(queue_mtx_);

    BufferingCache::CacheBuf* buf = cache_.CacheMalloc(dataSize);
    buf->Assign(dataBuf, dataSize, timestamp-base_timestamp_+CACHE_DELAY_MILLSECS);

    buf_queue_.push_back(buf);
}

void AudioBufferingQueue::ThreadRun()
{
    //::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

    for (;;)
    {
        if (is_stopping_)
            break;

        if (time_counter_.LastTimestamp() == -1)
        {
            ::Sleep(10);
            continue;
        }

        bool is_need_break = false;
        {
            base::AutoLock lg(queue_mtx_);
            if (time_counter_.Get() >= buf_queue_.front()->timestamp_)
            {
                is_need_break = true;
            }
        }
        if (is_need_break)
        {
            break;
        }
        else
        {
            ::Sleep(10);
            continue;
        }
    }

    for (;;)
    {
        if (is_stopping_)
            break;

        long long should_timestamp = av_rescale(audio_frame_count_, 1000, sample_rate_) + CACHE_DELAY_MILLSECS;

        if (time_counter_.Get() >= should_timestamp)
        {
            std::deque<BufferingCache::CacheBuf*> audio_bufs;
            {
                base::AutoLock lg(queue_mtx_);

                if (false == buf_queue_.empty())
                {
                    BufferingCache::CacheBuf* audio_buf = buf_queue_.front();
                    buf_queue_.pop_front();
                    audio_bufs.push_back(audio_buf);

                    audio_frame_count_+= 1024;
                }
            }

            if (is_stopping_) break;

            while (false == audio_bufs.empty())
            {
                BufferingCache::CacheBuf* audio_buf = audio_bufs.front();
                audio_bufs.pop_front();

                observer_->OnAudioQueueData(audio_buf->buf_, audio_buf->buflen_);

                base::AutoLock lg(queue_mtx_);
                cache_.CacheFree(audio_buf);
            }
        }

        ::Sleep(1);
    }
}

//////////////////////////////////////////////////////////////////////////

VideoBufferingQueue::VideoBufferingQueue(Observer* ob)
{
    is_stopping_ = false;

    observer_ = ob;
    base_timestamp_ = -1;
}

VideoBufferingQueue::~VideoBufferingQueue()
{

}

void VideoBufferingQueue::Start()
{
    ThreadStart();
}

void VideoBufferingQueue::Stop()
{
    if (is_stopping_) return;

    is_stopping_ = true;

    ThreadStop();
    ThreadJoin();

    while (false == buf_queue_.empty())
    {
        BufferingCache::CacheBuf* buf = buf_queue_.front();
        buf_queue_.pop_front();

        cache_.CacheFree(buf);
    }
}

void VideoBufferingQueue::Clear()
{
    base::AutoLock lg(queue_mtx_);

    base_timestamp_ = -1;

    while (false == buf_queue_.empty())
    {
        BufferingCache::CacheBuf* buf = buf_queue_.front();
        buf_queue_.pop_front();

        cache_.CacheFree(buf);
    }
}

void VideoBufferingQueue::Post(const char* dataBuf, int dataSize,
    long long timestamp)
{
    if (is_stopping_) return;

    if (time_counter_.LastTimestamp() == -1)
    {
        time_counter_.Reset();
    }

    if (base_timestamp_ == -1)
    {
        base_timestamp_ = timestamp;
    }

    base::AutoLock lg(queue_mtx_);

    BufferingCache::CacheBuf* buf = cache_.CacheMalloc(dataSize);
    buf->Assign(dataBuf, dataSize, timestamp-base_timestamp_+CACHE_DELAY_MILLSECS);

    buf_queue_.push_back(buf);

    SIMPLE_LOG("queue len: %d\n", buf_queue_.size());
}

void VideoBufferingQueue::ThreadRun()
{
    //::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

    for (;;)
    {
        if (is_stopping_)
            break;

        if (time_counter_.LastTimestamp() == -1)
        {
            ::Sleep(10);
            continue;
        }
        else
        {
            break;
        }
    }

    SIMPLE_LOG("go\n");

    for (;;)
    {
        if (is_stopping_)
            break;

        std::deque<BufferingCache::CacheBuf*> video_bufs;
        {
            base::AutoLock lg(queue_mtx_);
            while (false == buf_queue_.empty())
            {
                 //buf_queue_.swap(video_bufs);
                 BufferingCache::CacheBuf* buf = buf_queue_.front();
                 long long now = time_counter_.Get();
                 if (now >= buf->timestamp_)
                 {
                     buf_queue_.pop_front();
                     video_bufs.push_back(buf);
                 }
                 else
                 {
                     break;
                 }
            }
        }

        while (false == video_bufs.empty())
        {
            BufferingCache::CacheBuf* video_buf = video_bufs.front();
            video_bufs.pop_front();

            observer_->OnVideoQueueData(video_buf->buf_, video_buf->buflen_,
                video_buf->timestamp_);

            base::AutoLock lg(queue_mtx_);
            cache_.CacheFree(video_buf);
        }

        ::Sleep(10);
    }
}

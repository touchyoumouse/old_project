/*******************************************************************************
 * service_buffer.cpp
 * Copyright: (c) 2014 Haibin Du(haibindev.cnblogs.com). All rights reserved.
 * -----------------------------------------------------------------------------
 *
 * -----------------------------------------------------------------------------
 * 2015-7-18 18:35 - Created (Haibin Du)
 ******************************************************************************/

#include "service_buffer.h"

#include <algorithm>

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
}

ServiceBuffer::ServiceBuffer()
{
    sws_ctx_ = NULL;
    src_width_ = 0;
    src_height_ = 0;
    out_width_ = 0;
    out_height_ = 0;

    rgb_frame_ = NULL;
    is_has_data_ = false;
}

ServiceBuffer::~ServiceBuffer()
{
    Clear();
}

void ServiceBuffer::Init(int width, int height, int pixBytes,
    int outWidth, int outHeight)
{
    Clear();

    src_width_ = width;
    src_height_ = height;

    if (outWidth > 0) out_width_ = outWidth;
    else out_width_ = width;

    if (outHeight > 0) out_height_ = outHeight;
    else out_height_ = height;

    if (out_width_ == 0)
        out_width_ = width;
    if (out_height_ == 0)
        out_height_ = height;

    sws_ctx_ = sws_getContext(width, height, AV_PIX_FMT_YUV420P,
        out_width_, out_height_, PIX_FMT_RGB32,
        SWS_BICUBIC, 0, 0, 0);

    rgb_buf_.resize(out_width_ * out_height_ * pixBytes);

    rgb_frame_ = av_frame_alloc();
    avpicture_fill((AVPicture *)rgb_frame_, (uint8_t*)&rgb_buf_[0],
        AV_PIX_FMT_RGB32,  out_width_, out_height_);
}

void ServiceBuffer::UpdateOutInfo(int outWidth, int outHeight)
{
    if (sws_ctx_)
    {
        Init(src_width_, src_height_, 4, outWidth, outHeight);
    }
    else
    {
        out_width_ = outWidth;
        out_height_ = outHeight;
    }
}

void ServiceBuffer::Clear()
{
    if (sws_ctx_)
    {
        sws_freeContext(sws_ctx_);
        sws_ctx_ = NULL;
    }

    if (rgb_frame_)
    {
        av_frame_free(&rgb_frame_);
    }

    is_has_data_ = false;
}

void ServiceBuffer::MakeRgbBuf(AVFrame* yuvFrame)
{
    if (sws_ctx_ && yuvFrame && rgb_frame_)
    {
        sws_scale(sws_ctx_, yuvFrame->data, yuvFrame->linesize, 0, 
            src_height_, rgb_frame_->data, rgb_frame_->linesize);

        is_has_data_ = true;
    }
}

//////////////////////////////////////////////////////////////////////////

AVFrameCache::AVFrameCache()
{

}

AVFrameCache::~AVFrameCache()
{
    Clear();
}

AVFrame* AVFrameCache::MallocAndCopyAVFrame(AVFrame* baseFrame)
{
    AVFrame* new_frame = NULL;

    if (cache_list_.empty())
    {
        new_frame = av_frame_clone(baseFrame);
    }
    else
    {
        new_frame = cache_list_.front();
        cache_list_.pop_front();

        av_frame_copy(new_frame, baseFrame);
    }

    using_cache_list_.insert(new_frame);

    return new_frame;
}

void AVFrameCache::FreeAVFrame(AVFrame* frame)
{
    using_cache_list_.erase(frame);

    cache_list_.push_back(frame);
}

void AVFrameCache::Clear()
{
    for (auto it = cache_list_.begin(); it != cache_list_.end(); ++it)
    {
        AVFrame* frame = *it;

        av_frame_free(&frame);
    }
    cache_list_.clear();

    for (auto it = using_cache_list_.begin(); it != using_cache_list_.end(); ++it)
    {
        AVFrame* frame = *it;

        av_frame_free(&frame);
    }
    using_cache_list_.clear();
}

int AVFrameCache::CacheCount()
{
    return cache_list_.size() + using_cache_list_.size();
}

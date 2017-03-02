/*******************************************************************************
 * rgb_scale_convert.cpp
 * Copyright: (c) 2014 Haibin Du(haibindev.cnblogs.com). All rights reserved.
 * -----------------------------------------------------------------------------
 *
 * -----------------------------------------------------------------------------
 * 2016-2-16 9:29 - Created (Haibin Du)
 ******************************************************************************/

#include "rgb_scale_convert.h"

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
}

RgbScaleConvert::RgbScaleConvert()
{
    sws_ctx_ = NULL;
    src_width_ = 0;
    src_height_ = 0;
    out_width_ = 0;
    out_height_ = 0;

    src_rgb_frame_ = NULL;
    rgb_frame_ = NULL;
}

RgbScaleConvert::~RgbScaleConvert()
{
    Clear();
}

void RgbScaleConvert::Init(int srcWidth, int srcHeight,
    int outWidth, int outHeight)
{
    Clear();

    src_width_ = srcWidth;
    src_height_ = srcHeight;

    if (outWidth > 0)
        out_width_ = outWidth;
    if (outHeight > 0)
        out_height_ = outHeight;

    if (out_width_ == 0)
        out_width_ = src_width_;
    if (out_height_ == 0)
        out_height_ = src_height_;

    sws_ctx_ = sws_getContext(src_width_, src_height_, PIX_FMT_RGB32,
        out_width_, out_height_, PIX_FMT_RGB32,
        SWS_BICUBIC, 0, 0, 0);

    src_rgb_buf_.resize(src_width_ * src_height_ * 4);
    src_rgb_frame_ = av_frame_alloc();
//     avpicture_fill((AVPicture *)src_rgb_frame_, (uint8_t*)&src_rgb_buf_[0],
//         AV_PIX_FMT_RGB32,  src_width_, src_height_);

    rgb_buf_.resize(out_width_ * out_height_ * 4);
    rgb_frame_ = av_frame_alloc();
    avpicture_fill((AVPicture *)rgb_frame_, (uint8_t*)&rgb_buf_[0],
        AV_PIX_FMT_RGB32,  out_width_, out_height_);
}

void RgbScaleConvert::Clear()
{
    if (sws_ctx_)
    {
        sws_freeContext(sws_ctx_);
        sws_ctx_ = NULL;
    }

    if (src_rgb_frame_)
    {
        av_frame_free(&src_rgb_frame_);
    }

    if (rgb_frame_)
    {
        av_frame_free(&rgb_frame_);
    }
}

void RgbScaleConvert::ConvertRgbBuf(const char* srcRgbBuf, int srcRgbSize)
{
    if (srcRgbSize != src_rgb_buf_.size())
    {
        return;
    }

    avpicture_fill((AVPicture *)src_rgb_frame_, (uint8_t*)srcRgbBuf,
        AV_PIX_FMT_RGB32,  src_width_, src_height_);

    int ret = sws_scale(sws_ctx_, src_rgb_frame_->data, src_rgb_frame_->linesize, 0, 
        src_height_, rgb_frame_->data, rgb_frame_->linesize);
    int a= ret;
}


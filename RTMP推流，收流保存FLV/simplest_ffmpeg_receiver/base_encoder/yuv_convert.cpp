/*******************************************************************************
 * yuv_convert.cpp
 * Copyright: (c) 2014 Haibin Du(haibindev.cnblogs.com). All rights reserved.
 * -----------------------------------------------------------------------------
 *
 * -----------------------------------------------------------------------------
 * 2015-6-4 23:24 - Created (Haibin Du)
 ******************************************************************************/

#include "yuv_convert.h"

#if defined(HDEV_YUV_USE_FFMPEG)
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
}
#endif

YuvConvert::YuvConvert(int width, int height)
{
#if defined(HDEV_YUV_USE_FFMPEG)
    sws_ctx_ = NULL;
    src_frame_ = av_frame_alloc();
    yuv_frame_ = av_frame_alloc();
#else
#endif
    width_ = width;
    height_ = height;

    live_yuvsize_ = width_*height_*3/2;
    live_yuvbuf_ = new char[live_yuvsize_];
}

YuvConvert::~YuvConvert()
{
    delete[] live_yuvbuf_;

#if defined(HDEV_YUV_USE_FFMPEG)
    av_frame_free(&src_frame_);
    av_frame_free(&yuv_frame_);

    sws_freeContext(sws_ctx_);
#else
#endif
}

void YuvConvert::InitYuvFrame(bool isReverse)
{
#if defined(HDEV_YUV_USE_FFMPEG)
    avpicture_fill((AVPicture *)yuv_frame_, (uint8_t*)live_yuvbuf_,
        PIX_FMT_YUV420P, width_, height_);

    if (isReverse)
    {
        // 图像反转问题
        yuv_frame_->data[0] += yuv_frame_->linesize[0] * (height_ - 1);
        yuv_frame_->linesize[0] *= -1;                      
        yuv_frame_->data[1] += yuv_frame_->linesize[1] * (height_ / 2 - 1);
        yuv_frame_->linesize[1] *= -1;
        yuv_frame_->data[2] += yuv_frame_->linesize[2] * (height_ / 2 - 1);
        yuv_frame_->linesize[2] *= -1;
    }
#endif
}

void YuvConvert::InitFromRGB24(bool isReverse)
{
#if defined(HDEV_YUV_USE_FFMPEG)
    InitYuvFrame(isReverse);

    sws_ctx_ = sws_getContext(width_, height_, AV_PIX_FMT_BGR24,
        width_, height_, AV_PIX_FMT_YUV420P,
        SWS_BILINEAR, 0, 0, 0);
#else
#endif
}

void YuvConvert::ConvertFromRGB24(const char* dataBuf, int dataSize)
{
#if defined(HDEV_YUV_USE_FFMPEG)
    avpicture_fill((AVPicture *)src_frame_, (uint8_t*)dataBuf, AV_PIX_FMT_BGR24, 
        width_, height_);
    sws_scale(sws_ctx_, src_frame_->data, src_frame_->linesize, 0, 
        height_, yuv_frame_->data, yuv_frame_->linesize);
#else
    Rgb24ToYUV420(dataBuf, width_, height_, live_yuvbuf_);
#endif
}

void YuvConvert::InitFromRGB32(bool isReverse)
{
#if defined(HDEV_YUV_USE_FFMPEG)
    InitYuvFrame(isReverse);

    sws_ctx_ = sws_getContext(width_, height_, AV_PIX_FMT_RGB32,
        width_, height_, AV_PIX_FMT_YUV420P,
        SWS_BILINEAR, 0, 0, 0);
#else

#endif
}

void YuvConvert::ConvertFromRGB32(const char* dataBuf, int dataSize)
{
#if defined(HDEV_YUV_USE_FFMPEG)
    avpicture_fill((AVPicture *)src_frame_, (uint8_t*)dataBuf, AV_PIX_FMT_RGB32, 
        width_, height_);
    sws_scale(sws_ctx_, src_frame_->data, src_frame_->linesize, 0, 
        height_, yuv_frame_->data, yuv_frame_->linesize);
#else
    Rgb32ToYUV420(dataBuf, width_, height_, live_yuvbuf_);
#endif
}

void YuvConvert::InitFromYUY2(bool isReverse)
{
#if defined(HDEV_YUV_USE_FFMPEG)
    InitYuvFrame(isReverse);

    sws_ctx_ = sws_getContext(width_, height_, AV_PIX_FMT_YUYV422,
        width_, height_, AV_PIX_FMT_YUV420P,
        SWS_BICUBIC, 0, 0, 0);
#else

#endif
}

void YuvConvert::ConvertFromYUY2(const char* dataBuf, int dataSize)
{
#if defined(HDEV_YUV_USE_FFMPEG)
    avpicture_fill((AVPicture *)src_frame_, (uint8_t*)dataBuf, AV_PIX_FMT_YUYV422, 
        width_, height_);
    sws_scale(sws_ctx_, src_frame_->data, src_frame_->linesize, 0, 
        height_, yuv_frame_->data, yuv_frame_->linesize);
#else
    Yuv2ToYUV420(dataBuf, width_, height_, live_yuvbuf_);
#endif
}

void YuvConvert::Rgb24ToYUV420(const char* rgbBuf, int width, int height, char* yuvBuf)
{
    char* bufY = yuvBuf; 
    char* bufU = yuvBuf + width * height; 
    char* bufV = bufU + (width * height* 1/4); 

    unsigned char y, u, v, r, g, b; 
    unsigned int ylen = width * height;
    unsigned int ulen = (width * height)/4;
    unsigned int vlen = (width * height)/4; 
    for (int j = 0; j<height;j++)
    {
        const char* bufRGB = rgbBuf + width * (height - 1 - j) * 3 ; 
        for (int i = 0;i<width;i++)
        {
            int pos = width * i + j;
            b = *(bufRGB++);
            g = *(bufRGB++);
            r = *(bufRGB++);

            y = (unsigned char)( ( 66 * r + 129 * g + 25 * b + 128) >> 8) + 16 ;           
            u = (unsigned char)( ( -38 * r - 74 * g + 112 * b + 128) >> 8) + 128 ;           
            v = (unsigned char)( ( 112 * r - 94 * g - 18 * b + 128) >> 8) + 128 ;
            *(bufY++) = MY_MAX( 0, MY_MIN(y, 255 ));

            if (j%2==0&&i%2 ==0)
            {
                if (u>255) u=255;
                if (u<0)   u = 0;
                *(bufU++) = u;
                //存u分量
            }
            else
            {
                //存v分量
                if (i%2==0)
                {
                    if (v>255) v = 255;
                    if (v<0)   v = 0;
                    *(bufV++) = v;
                }
            }
        }
    }
}

void YuvConvert::Rgb32ToYUV420(const char* rgbBuf, int width, int height, char* yuvBuf)
{
    char* bufY = yuvBuf; 
    char* bufU = yuvBuf + width * height; 
    char* bufV = bufU + (width * height* 1/4);

    unsigned char y, u, v, r, g, b; 
    for (int j = 0; j<height;j++)
    {
        const char* bufRGB = rgbBuf + width * (height - 1 - j) * 4 ; 
        for (int i = 0;i<width;i++)
        {
            int pos = width * i + j;
            b = *(bufRGB++);
            g = *(bufRGB++);
            r = *(bufRGB++);
            bufRGB++;   // a

            y = (unsigned char)( ( 66 * r + 129 * g + 25 * b + 128) >> 8) + 16 ;           
            u = (unsigned char)( ( -38 * r - 74 * g + 112 * b + 128) >> 8) + 128 ;           
            v = (unsigned char)( ( 112 * r - 94 * g - 18 * b + 128) >> 8) + 128 ;
            *(bufY++) = max( 0, min(y, 255 ));

            if (j%2==0&&i%2 ==0)
            {
                if (u>255) u=255;
                if (u<0) u = 0;
                *(bufU++) =u;
            }
            else
            {
                //存v分量
                if (i%2==0)
                {
                    if (v>255) v = 255;
                    if (v<0) v = 0;
                    *(bufV++) =v;
                }
            }
        }
    }
}

void YuvConvert::Yuv2ToYUV420(const char* yuy2Buf, int width, int height, char* i420Buf)
{
    const char* pbuf = yuy2Buf;
    char* py = i420Buf;
    char* pu = i420Buf + width*height;
    char* pv = pu + width*height/4;

    for (int j = 0; j < height; j++)
    {
        for (int i = 0; i < width; i+=2)
        {
            *py = pbuf[0];    // Y0
            py++;
            *py = pbuf[2];    // Y1
            py++;

            if (j%2==0&&i%2 ==0)
            {
                *pu = pbuf[1];    // U
                pu++;
            }
            else
            {
                if (i%2==0)
                {
                    *pv = pbuf[3];    // V
                    pv++;
                }
            }
            pbuf += 4;
        }
    }
}

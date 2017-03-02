/*******************************************************************************
 * yuv_convert.h
 * Copyright: (c) 2014 Haibin Du(haibindev.cnblogs.com). All rights reserved.
 * -----------------------------------------------------------------------------
 *
 * 各种原始像素格式到YUV420P转换的算法，可以选择使用ffmpeg swscale来转换，
 * 需要定义 *HDEV_YUV_USE_FFMPEG*
 *
 * -----------------------------------------------------------------------------
 * 2015-6-4 23:24 - Created (Haibin Du)
 ******************************************************************************/

#ifndef _HDEV_YUV_CONVERT_H_
#define _HDEV_YUV_CONVERT_H_

#include "base/base.h"

//#define HDEV_YUV_USE_FFMPEG

#if defined(HDEV_YUV_USE_FFMPEG)
struct SwsContext;
struct AVFrame;
#endif

class YuvConvert
{
public:
    YuvConvert(int width, int height);

    ~YuvConvert();

    void InitFromRGB24(bool isReverse);

    void ConvertFromRGB24(const char* dataBuf, int dataSize);

    void InitFromRGB32(bool isReverse);

    void ConvertFromRGB32(const char* dataBuf, int dataSize);

    // YUYV
    void InitFromYUY2(bool isReverse);

    void ConvertFromYUY2(const char* dataBuf, int dataSize);

    char* YuvBuf() { return live_yuvbuf_; }

    int YuvSize() { return live_yuvsize_; }

private:
    void InitYuvFrame(bool isReverse);

    void Rgb24ToYUV420(const char* rgbBuf, int width, int height, char* yuvBuf);

    void Rgb32ToYUV420(const char* rgbBuf, int width, int height, char* yuvBuf);

    void Yuv2ToYUV420(const char* yuy2Buf, int width, int height, char* i420Buf);

private:
#if defined(HDEV_YUV_USE_FFMPEG)
    SwsContext* sws_ctx_;
    AVFrame* src_frame_;
    AVFrame* yuv_frame_;
#else
#endif

    int width_;
    int height_;

    char* live_yuvbuf_;
    int live_yuvsize_;
};

#endif // _HDEV_YUV_CONVERT_H_

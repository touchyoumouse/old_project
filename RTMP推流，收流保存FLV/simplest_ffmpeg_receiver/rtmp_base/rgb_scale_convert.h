/*******************************************************************************
 * rgb_scale_convert.h
 * Copyright: (c) 2014 Haibin Du(haibindev.cnblogs.com). All rights reserved.
 * -----------------------------------------------------------------------------
 *
 * 对RGB画面进行缩放
 *
 * -----------------------------------------------------------------------------
 * 2016-2-16 9:29 - Created (Haibin Du)
 ******************************************************************************/

#ifndef _HDEV_RGB_SCALE_CONVERT_H_
#define _HDEV_RGB_SCALE_CONVERT_H_

#include <deque>
#include <set>
#include <vector>

struct SwsContext;
struct AVFrame;

//////////////////////////////////////////////////////////////////////////
// 输入源最终变换数据

class RgbScaleConvert
{
public:
    RgbScaleConvert();

    ~RgbScaleConvert();

    /***
     * 初始化
     * @param srcWidth: 源宽度 
     * @param srcHeight: 源高度
     * @param outWidth: 输出宽度，如果为0，表示和源相同
     * @param outHeight: 输出高度，如果为0，表示和源相同
     */
    void Init(int srcWidth, int srcHeight,
        int outWidth, int outHeight);

    void Clear();

    // 生成输出的RGB数据
    void ConvertRgbBuf(const char* srcRgbBuf, int srcRgbSize);

    int SrcWidth() { return src_width_; }

    int SrcHeight() { return src_height_; }

    int OutWidth() { return out_width_; }

    int OutHeight() { return out_height_; }

    AVFrame* GetRgbFrame() { return rgb_frame_; }

    char* GetRgbBuf() { return &rgb_buf_[0]; }

    int GetRgbSize() { return rgb_buf_.size();}

private:
    SwsContext* sws_ctx_;
    int src_width_;
    int src_height_;
    int out_width_;
    int out_height_;

    AVFrame* src_rgb_frame_;        // 输入RGB画面
    std::vector<char> src_rgb_buf_; // 输入RGB画面的数据

    AVFrame* rgb_frame_;        // 输出RGB画面
    std::vector<char> rgb_buf_; // 输出RGB画面的数据
};

#endif // _HDEV_RGB_SCALE_CONVERT_H_

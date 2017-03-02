/*******************************************************************************
 * rgb_scale_convert.h
 * Copyright: (c) 2014 Haibin Du(haibindev.cnblogs.com). All rights reserved.
 * -----------------------------------------------------------------------------
 *
 * ��RGB�����������
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
// ����Դ���ձ任����

class RgbScaleConvert
{
public:
    RgbScaleConvert();

    ~RgbScaleConvert();

    /***
     * ��ʼ��
     * @param srcWidth: Դ��� 
     * @param srcHeight: Դ�߶�
     * @param outWidth: �����ȣ����Ϊ0����ʾ��Դ��ͬ
     * @param outHeight: ����߶ȣ����Ϊ0����ʾ��Դ��ͬ
     */
    void Init(int srcWidth, int srcHeight,
        int outWidth, int outHeight);

    void Clear();

    // ���������RGB����
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

    AVFrame* src_rgb_frame_;        // ����RGB����
    std::vector<char> src_rgb_buf_; // ����RGB���������

    AVFrame* rgb_frame_;        // ���RGB����
    std::vector<char> rgb_buf_; // ���RGB���������
};

#endif // _HDEV_RGB_SCALE_CONVERT_H_

/*******************************************************************************
 * service_buffer.h
 * Copyright: (c) 2014 Haibin Du(haibindev.cnblogs.com). All rights reserved.
 * -----------------------------------------------------------------------------
 *
 * �洢��Ƶ�������ݣ������������ź�ת��
 *
 * -----------------------------------------------------------------------------
 * 2015-7-18 18:35 - Created (Haibin Du)
 ******************************************************************************/

#ifndef _HDEV_SERVICE_BUFFER_H_
#define _HDEV_SERVICE_BUFFER_H_

#include <deque>
#include <set>
#include <vector>

struct SwsContext;
struct AVFrame;

//////////////////////////////////////////////////////////////////////////
// ����Դ���ձ任����

class ServiceBuffer
{
public:
    ServiceBuffer();

    ~ServiceBuffer();

    /***
     * ��ʼ��
     * @param width: Դ��� 
     * @param height: Դ�߶�
     * @param pixBytes: ���RGB���ֽ�����24��32��
     * @param outWidth: �����ȣ����Ϊ0����ʾ��Դ��ͬ
     * @param outHeight: ����߶ȣ����Ϊ0����ʾ��Դ��ͬ
     */
    void Init(int width, int height, int pixBytes,
        int outWidth, int outHeight);

    /***
     * ����ת���������
     * @param 
     * @param 
     * @returns:
     */
    void UpdateOutInfo(int outWidth, int outHeight);

    void Clear();

    // ���������RGB����
    void MakeRgbBuf(AVFrame* yuvFrame);

    int SrcWidth() { return src_width_; }

    int SrcHeight() { return src_height_; }

    int OutWidth() { return out_width_; }

    int OutHeight() { return out_height_; }

    AVFrame* GetRgbFrame() { return rgb_frame_; }

    char* GetRgbBuf() { return &rgb_buf_[0]; }

    int GetRgbSize() { return rgb_buf_.size();}

    bool IsHasData() { return is_has_data_; }

private:
    SwsContext* sws_ctx_;
    int src_width_;
    int src_height_;
    int out_width_;
    int out_height_;

    AVFrame* rgb_frame_;        // ���RGB����
    std::vector<char> rgb_buf_; // ���RGB���������
    bool is_has_data_;
};

//////////////////////////////////////////////////////////////////////////
// ����ԴYUV֡�ڴ��

class AVFrameCache
{
public:
    AVFrameCache();

    ~AVFrameCache();

    AVFrame* MallocAndCopyAVFrame(AVFrame* baseFrame);

    void FreeAVFrame(AVFrame* frame);

    void Clear();

    int CacheCount();

private:
    std::deque<AVFrame*> cache_list_;
    std::set<AVFrame*> using_cache_list_;
};

#endif // _HDEV_SERVICE_BUFFER_H_

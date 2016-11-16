/*******************************************************************************
 * service_buffer.h
 * Copyright: (c) 2014 Haibin Du(haibindev.cnblogs.com). All rights reserved.
 * -----------------------------------------------------------------------------
 *
 * 存储视频画面数据，并做画面缩放和转换
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
// 输入源最终变换数据

class ServiceBuffer
{
public:
    ServiceBuffer();

    ~ServiceBuffer();

    /***
     * 初始化
     * @param width: 源宽度 
     * @param height: 源高度
     * @param pixBytes: 输出RGB的字节数（24，32）
     * @param outWidth: 输出宽度，如果为0，表示和源相同
     * @param outHeight: 输出高度，如果为0，表示和源相同
     */
    void Init(int width, int height, int pixBytes,
        int outWidth, int outHeight);

    /***
     * 更新转换输出参数
     * @param 
     * @param 
     * @returns:
     */
    void UpdateOutInfo(int outWidth, int outHeight);

    void Clear();

    // 生成输出的RGB数据
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

    AVFrame* rgb_frame_;        // 输出RGB画面
    std::vector<char> rgb_buf_; // 输出RGB画面的数据
    bool is_has_data_;
};

//////////////////////////////////////////////////////////////////////////
// 输入源YUV帧内存池

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

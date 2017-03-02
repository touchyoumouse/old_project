/*******************************************************************************
 * avi_muxer.h
 * Copyright: (c) 2014 Haibin Du(haibindev.cnblogs.com). All rights reserved.
 * -----------------------------------------------------------------------------
 *
 * JPEG编码保存avi格式，音频用pcm
 *
 * -----------------------------------------------------------------------------
 * 2015-2-13 16:54 - Created (Haibin Du)
 ******************************************************************************/

#ifndef _HDEV_AVI_MUXER_H_
#define _HDEV_AVI_MUXER_H_

#include <string>
#include <vector>

#include "base/Lock.h"
#include "base/TimeCounter.h"
#include "record/ff_reader.h"

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
}

class FFAudioEncoder;
class FFVideoEncoder;
class FFWritter;
class AviMuxer
{
public:
    /***
     * 构造函数
     * @param filename: 要保存的文件名
     */
    explicit AviMuxer(const std::string& filename);

    ~AviMuxer();

    void SetOpeningFile(const std::string& filename,
        int opendingMillsecs);

    void SetEndingFile(const std::string& filename,
        int endingMillsecs);

    /***
     * 创建文件
     * @param width: 宽度
     * @param height: 高度
     * @param fps: 视频帧率
     * @param bitrate: 视频码率
     * @param samrate: 采样率
     * @param channel: 声道
     */
    void Open(int width, int height, int fps, int bitrate, 
        int samrate, int channel);

    void Close();

    void SetPause(bool isPause);

    void OnPcmData(const char* dataBuf, int dataSize);

    // rgb32接口
    void OnRgbData(const char* dataBuf, int dataSize, long long shouldTS);

    // yuv420p接口
    void OnYuvData(const char* dataBuf, int dataSize, long long shouldTS);

private:
    void AppendOpening();

    void AppendEnding();

    void Appending(bool isOpening);

    void AppendingVideo(FFReader& ffReader, bool isOpening);

    void AppendingPicture(FFReader& ffReader, bool isOpening);

    void AppendingZeroPCM(long long pcmMillsecs, bool isOpening);

private:
    // avi格式构造
    FFWritter* avi_writter_;
    int width_;
    int height_;
    int fps_;
    int bitrate_;
    int samrate_;
    int channel_;

    // 音频编码器（这里没有用到，因为是直接写入PCM）
    std::vector<unsigned char> audio_buf_;
    FFAudioEncoder* audio_encoder_;

    // MJPEG视频编码器
    std::vector<char> mjpeg_buf_;
    FFVideoEncoder* video_encoder_;

    std::string avi_name_;

    std::string opening_name_;
    int opening_millsecs_;
    std::string ending_name_;
    int ending_millsecs_;
    
    TimeCounter time_counter_;

    base::Lock write_lock_;
};

#endif // _HDEV_AVI_MUXER_H_

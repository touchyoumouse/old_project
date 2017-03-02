/*******************************************************************************
 * record_thread.h
 * Copyright: (c) 2014 Haibin Du(haibindev.cnblogs.com). All rights reserved.
 * -----------------------------------------------------------------------------
 *
 *
 *
 * -----------------------------------------------------------------------------
 * 2016-2-25 21:41 - Created (Haibin Du)
 ******************************************************************************/

#ifndef _HDEV_RECORD_THREAD_H_
#define _HDEV_RECORD_THREAD_H_

#include "base/base.h"

#include <deque>
#include <string>

#include "base/DataBuffer.h"
#include "base/Lock.h"
#include "base/SimpleThread.h"
#include "base/TimeCounter.h"

class AviMuxer;
class Recorder;

class RecordThread : public base::SimpleThread
{
public:
    /***
     * 初始化
     * @param savePath: 保存文件绝对路径, 不带扩展名
     * @param isSaveAvi: 是否保存AVI文件
     * @param isSaveMp4: 是否保存MP4文件
     * @param isSaveFlv: 是否保存FLV文件
     */
    RecordThread(const std::string& savePath,
        bool isSaveAvi, bool isSaveMp4, bool isSaveFlv);

    ~RecordThread();

    /***
     * 设置录制文件的片头片尾
     */
    void SetOpeningEnding(const std::string& openAviName, const std::string& openMp4Name,
        const std::string& openFlvName, int openingMillsecs,
        const std::string& endAviName, const std::string& endMp4Name,
        const std::string& endFlvName, int endingMillsecs);

    /***
     * 开始录制
     * @param isHasAudio: 是否有音频
     * @param sampleRate: 采样率
     * @param channel: 声道
     * @param audioBitrate: 音频码率
     * @param width: 宽度
     * @param height: 高度
     * @param fps: 视频帧率
     * @param videoBitrate: 视频码率
     * @param bitrateMp4: h264视频码率
     * @returns:
     */
    void Start(bool isHasAudio, int sampleRate, int channel, int audioBitrate,
        int width, int height, int fps, int videoBitrate, int bitrateMp4,
        int quality, bool isMp4UseCBR);

    /***
     * 结束录制
     */
    void Stop();

    /***
     * 暂停录制
     */
    void Pause();

    /***
     * 写入PCM原始音频数据
     * @param pcmBuf: 音频数据地址
     * @param bufSize: 数据大小
     */
    void OnPcmBufferToAvi(const char* pcmBuf, int bufSize);

    /***
     * 写入一帧原始视频画面，RGB格式
     * @param rgbBuf: RGB32视频画面
     * @param bufSize: 数据大小
     */
    void OnRgbBufferToAvi(const char* rgbBuf, int bufSize, long long shouldTS);

    void OnH264Extra(const char* extraBuf, int extraSize);

    void OnH264Frame(const char* frameBuf, int frameSize,
        bool isKeyframe, long long shouldTS);

    void OnAACExtra(const char* extraBuf, int extraSize);

    void OnAACFrame(const char* frameBuf, int frameSize);

    bool IsRecording() { return is_recording_; }

    // ------------------------------------------------------------------------
    // SimpleThread虚函数接口

    virtual void ThreadRun();

private:
    void CreateRecordFile();

private:
    std::string save_path_;
    bool is_save_avi_;
    bool is_save_mp4_;
    bool is_save_flv_;

    std::string opening_avi_;
    std::string opening_mp4_;
    std::string opening_flv_;
    int opening_millsecs_;
    std::string ending_avi_;
    std::string ending_mp4_;
    std::string ending_flv_;
    int ending_millsecs_;

    bool is_has_audio_;
    int samplerate_;
    int channle_;
    int audio_bitrate_;
    
    int width_;
    int height_;
    int fps_;
    int video_bitrate_;
    int bitrate_mp4_;
    int quality_;
    bool is_mp4_use_cbr_;

    // 录制
    Recorder* recorder_;
    char* aac_extra_data_;
    int aac_extra_size_;

    // AVI文件保存
    AviMuxer* avi_muxer_;

    base::Lock write_mtx_;

    TimeCounter time_counter_;

    volatile bool is_recording_;
    volatile bool is_pause_;
    volatile bool is_stop_;
};

#endif // _HDEV_RECORD_THREAD_H_

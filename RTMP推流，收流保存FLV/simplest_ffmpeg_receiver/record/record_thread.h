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
     * ��ʼ��
     * @param savePath: �����ļ�����·��, ������չ��
     * @param isSaveAvi: �Ƿ񱣴�AVI�ļ�
     * @param isSaveMp4: �Ƿ񱣴�MP4�ļ�
     * @param isSaveFlv: �Ƿ񱣴�FLV�ļ�
     */
    RecordThread(const std::string& savePath,
        bool isSaveAvi, bool isSaveMp4, bool isSaveFlv);

    ~RecordThread();

    /***
     * ����¼���ļ���ƬͷƬβ
     */
    void SetOpeningEnding(const std::string& openAviName, const std::string& openMp4Name,
        const std::string& openFlvName, int openingMillsecs,
        const std::string& endAviName, const std::string& endMp4Name,
        const std::string& endFlvName, int endingMillsecs);

    /***
     * ��ʼ¼��
     * @param isHasAudio: �Ƿ�����Ƶ
     * @param sampleRate: ������
     * @param channel: ����
     * @param audioBitrate: ��Ƶ����
     * @param width: ���
     * @param height: �߶�
     * @param fps: ��Ƶ֡��
     * @param videoBitrate: ��Ƶ����
     * @param bitrateMp4: h264��Ƶ����
     * @returns:
     */
    void Start(bool isHasAudio, int sampleRate, int channel, int audioBitrate,
        int width, int height, int fps, int videoBitrate, int bitrateMp4,
        int quality, bool isMp4UseCBR);

    /***
     * ����¼��
     */
    void Stop();

    /***
     * ��ͣ¼��
     */
    void Pause();

    /***
     * д��PCMԭʼ��Ƶ����
     * @param pcmBuf: ��Ƶ���ݵ�ַ
     * @param bufSize: ���ݴ�С
     */
    void OnPcmBufferToAvi(const char* pcmBuf, int bufSize);

    /***
     * д��һ֡ԭʼ��Ƶ���棬RGB��ʽ
     * @param rgbBuf: RGB32��Ƶ����
     * @param bufSize: ���ݴ�С
     */
    void OnRgbBufferToAvi(const char* rgbBuf, int bufSize, long long shouldTS);

    void OnH264Extra(const char* extraBuf, int extraSize);

    void OnH264Frame(const char* frameBuf, int frameSize,
        bool isKeyframe, long long shouldTS);

    void OnAACExtra(const char* extraBuf, int extraSize);

    void OnAACFrame(const char* frameBuf, int frameSize);

    bool IsRecording() { return is_recording_; }

    // ------------------------------------------------------------------------
    // SimpleThread�麯���ӿ�

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

    // ¼��
    Recorder* recorder_;
    char* aac_extra_data_;
    int aac_extra_size_;

    // AVI�ļ�����
    AviMuxer* avi_muxer_;

    base::Lock write_mtx_;

    TimeCounter time_counter_;

    volatile bool is_recording_;
    volatile bool is_pause_;
    volatile bool is_stop_;
};

#endif // _HDEV_RECORD_THREAD_H_

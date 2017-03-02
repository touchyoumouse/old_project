/*******************************************************************************
 * rtmp_publish.h
 * Copyright: (c) 2014 Haibin Du(haibindev.cnblogs.com). All rights reserved.
 * -----------------------------------------------------------------------------
 *
 * rtmp发布流，代码在rtmp_synth_service项目里的RtmpPub版本上改动
 *
 * -----------------------------------------------------------------------------
 * 2015-6-5 9:14 - Created (Haibin Du)
 ******************************************************************************/

#ifndef _HDEV_RTMP_PUBLISH_H_
#define _HDEV_RTMP_PUBLISH_H_

#include "base/base.h"

#include <deque>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "base/Lock.h"
#include "base/SimpleThread.h"
#include "base/TimeCounter.h"

struct RtmpBuffer
{
    enum
    {
        RTMP_DATA_AUDIO = 0x01,
        RTMP_DATA_VIDEO = 0x02,
    };

    int type_;
    long long timestamp_;
    bool is_keyframe_;

    std::vector<char>* buf_;
    int buf_capacity_;
    int data_size_;

    explicit RtmpBuffer(int capacity)
    {
        buf_capacity_ = capacity;
        buf_ = new std::vector<char>(buf_capacity_);
        data_size_ = 0;
    }

    ~RtmpBuffer()
    {
        delete buf_;
    }

    char* DataBuf() { return &(*buf_)[0]; }

    int DataSize() { return data_size_; }

    void SetSize(int dataSize) { data_size_ = dataSize; }

    void Clear() { data_size_ = 0; }

    void MakeSureCapacity(int needCapacity)
    {
        if (needCapacity > buf_capacity_)
        {
            delete buf_;
            buf_capacity_ = needCapacity;
            buf_ = new std::vector<char>(buf_capacity_);
            data_size_ = 0;
        }
    }
};

class LibRtmp;
class FlvWritter;
class RtmpPublish : public base::SimpleThread
{
public:
    enum RtmpPubState
    {
        RTMP_PUB_INIT,
        RTMP_PUB_CONNECTING,
        RTMP_PUB_CONNECT_SUCCEED,
        RTMP_PUB_CONNECT_FAILED,
        RTMP_PUB_SEND_FAILED,
    };

public:
    RtmpPublish(
        const std::string& rtmpUrl, // 发布rtmp流的地址(rtmp://x.x.x.x/live/streamname)
        bool isNeedLog,             // 是否需要librtmp打印日志
        int audioSampleRate,        // 音频采样率(一般为44100 22050 16000)
        int audioChannels,          // 音频声道数(一般为1 2)
        int videoWidth,             // 视频宽
        int videoHeight             // 视频高
        );

    ~RtmpPublish();

    virtual void ThreadRun();

    void Start();

    void Stop();

    void SetHasAudio(bool isHas) { is_has_audio_ = isHas; }

    void SetHasVideo(bool isHas) { is_has_video_ = isHas; }

    bool IsHasAudio() { return is_has_audio_; }

    bool IsHasVideo() { return is_has_video_; }

    // 数据

    void SetAACSpecificInfo(const char* infoBuf, int bufSize);

    void PostAACFrame(const char* buf, int bufLen);

    void PostH264Frame(const char* buf, int bufLen, long long shouldTS);

    // 录制flv

    void SetRecord(const std::string& recordDir, int recordInterval);

    void StartRecord();

    void StopRecord();

    // 状态

    RtmpPubState GetPubState() { return pub_state_; }

    bool IsStarted() { return is_started_; }

    bool IsNeedReConnect();

    void ClearSpsAndPps();

    void TrySendSequenceHeader();

    int SampleRate() { return source_samrate_; }

    int ChannelCount() { return source_channel_; }

    int Width() { return video_width_; }

    int Height() { return video_height_; }

private:
    void CheckBuflist_Locked();

    void ProcessAACBuffer(char* dataBuf, int dataSize, long long timestamp);

    void ProcessH264Buffer(char* dataBuf, int dataSize, long long timestamp, bool isKeyframe);

    void OnSPSAndPPS(char* spsBuf, int spsSize, char* ppsBuf, int ppsSize);

    void SendVideoData(char* buf, int bufLen, unsigned int timestamp, bool isKeyframe);

    void SendAudioData(char* buf, int bufLen, unsigned int timestamp);

    void SendMetadataPacket();

    void SendAVCSequenceHeaderPacket();

    void SendAACSequenceHeaderPacket();

    std::string GetRecordFilename(int year, int month, int day, int hour);

    bool IsNeedNextRecord(int nowHour);

    //////////////////////////////////////////////////////////////////////////
    // memory cache

    RtmpBuffer* CreateRtmpBuf(int dataSize, bool isVideo);

    void DeleteRtmpBuf(RtmpBuffer* rtmpBuf);

    std::map<RtmpBuffer*, bool> buf_memory_caches_;

private:
    LibRtmp* librtmp_;
    std::string rtmp_url_;

    // 录制相关
    FlvWritter* flv_writter_;
    std::string record_dir_;
    int last_record_hour_;
    int record_interval_;

    int source_samrate_;
    int source_channel_;
    int video_width_;
    int video_height_;

    //     char* x264_buf_;
    int x264_len_;
    char* sps_;        // sequence parameter set
    int sps_size_;
    char* pps_;        // picture parameter set
    int pps_size_;

    char* aac_info_buf_;
    int aac_info_size_;
    long long aac_frame_count_;

    TimeCounter timestamp_;

    std::deque<RtmpBuffer*> encode_buflist_;
    base::Lock list_mtx_;

    bool is_has_audio_;
    bool is_has_video_;

    RtmpPubState pub_state_;
    volatile bool is_started_;

    volatile bool is_recording_;

    bool has_flv_writter_header_;
    volatile bool is_header_send_;
};

#endif // _HDEV_RTMP_PUBLISH_H_

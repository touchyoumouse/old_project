/*******************************************************************************
 * source_reader.h
 * Copyright: (c) 2014 Haibin Du(haibindev.cnblogs.com). All rights reserved.
 * -----------------------------------------------------------------------------
 *
 * 输入源，调用相应协议读取数据，进行解码，反馈给上级
 *
 * -----------------------------------------------------------------------------
 * 2015-6-23 10:09 - Created (Haibin Du)
 ******************************************************************************/

#ifndef _HDEV_SOURCE_READER_H_
#define _HDEV_SOURCE_READER_H_

#include "base/base.h"

#include <vector>

#include "base/buf_unit_helper.h"
#include "base/Lock.h"
#include "base/SimpleThread.h"
#include "base/TimeCounter.h"

#include "ff_url_reader.h"
#include "rtmp_reader.h"
#include "reader_buffering.h"

#include "service/service_config.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libswresample/swresample.h"
#include "libavutil/opt.h"
}

class FFVideoDecoder;
class FAACDecoder;
class FFUrlReader;
class Recorder;
class RtmpPublisher;
class SourceReader
    : base::SimpleThread
    , public FFUrlReader::Observer
    , public RtmpReader::Observer
    , public AudioBufferingQueue::Observer
    , public VideoBufferingQueue::Observer
{
public:
    class Observer
    {
    public:
        virtual ~Observer() {}

        virtual void OnSourceReaderEvent(int sourceId, int eventCode) = 0;

        virtual void OnSourceReaderInfo(int sourceId, 
            int sampleRate, int channel,
            int width, int height) = 0;

        virtual void OnSourceReaderAudioBuf(int sourceId, 
            const char* dataBuf, int dataLen) = 0;

        virtual void OnSourceReaderVideoBuf(int sourceId, 
            AVFrame* avFrame, long long diffTime) = 0;
    };

public:
    /***
     * 构造函数
     * @param sourceId: 输入源ID
     * @param sourceType: 输入源协议类型
     * @param ob: 观察者（回调函数接收类）
     * @param serviceCfg: 主配置信息
     * @param srcCfg: 输入源配置信息
     * @returns:
     */
    explicit SourceReader(int sourceId, int sourceType, Observer* ob,
        const ServiceConfig& serviceCfg, const SourceConfig& srcCfg);

    ~SourceReader();

    /***
     * 开始获取源数据
     * @param requestUrl: 源地址
     * @returns:
     */
    void Start(const std::string& requestUrl);

    /***
     * 停止接收数据
     */
    void Stop();

    /***
     * 开始录制
     */
    void StartRecord();

    /***
     * 暂停录制
     */
    void PauseRecord();

    /***
     * 结束录制
     */
    void StopRecord();

    /***
     * 更新配置信息
     * @param serviceCfg: 主配置信息
     * @param srcCfg: 输入源配置信息
     * @returns:
     */
    void UpdateConfig(const ServiceConfig& serviceCfg,
        const SourceConfig& srcCfg);

    // 输入源是否出错
    bool IsFailed() { return is_failed_; }

    // 第一个音频数据到达了多久
    long long AudioArriveMillsecs();

    // 第一个视频数据到达了多久
    long long VideoArriveMillsecs();

    // -------------------------------------------
    // base::SimpleThread 回调函数

    virtual void ThreadRun();

    // -------------------------------------------
    // FFUrlReader::Observer 回调函数

    virtual void OnFFEvent(int eventId);

    virtual void OnFFCodecInfo(
        int audioCodec, int sampleRate, int channelCount,
        int videoCodec, int width, int height, int pixFmt,
        const char* extraBuf, int extraSize);

    virtual void OnFFAudioBuf(const char* dataBuf, int dataSize,
        long long timestamp, long long duration);

    virtual void OnFFVideoBuf(const char* dataBuf, int dataSize,
        long long timestamp, long long duration);

    virtual void OnFFAVPacket(AVPacket* avPacket,
        long long timestamp, long long duration);

    // -------------------------------------------
    // RtmpReader::Observer 回调函数

    virtual void OnRtmpErrCode(int errCode);

    virtual void OnRtmpAudioCodec(int samRate, int channel);

    virtual void OnRtmpVideoCodec(int width, int height);

    virtual void OnRtmpAudioBuf(const char* dataBuf, unsigned int dataLen,
        long long timestamp);

    virtual void OnRtmpVideoBuf(const char* dataBuf, unsigned int dataLen,
        long long timestamp);

    // -------------------------------------------
    // AudioBufferingQueue::Observer 回调函数

    virtual void OnAudioQueueData(const char* dataBuf, int dataSize);

    // -------------------------------------------
    // VideoBufferingQueue::Observer 回调函数

    virtual void OnVideoQueueData(const char* dataBuf, int dataSize,
        long long timestamp);

    /***
     * 定时检查输入源是否需要重连
     */
    void OnCheckTimer();

private:
    void CreateAudioDecoder(int audioCodec, int sampleRate, int channelCount);

    void CreateAudioResample(int toSampleRate);

    void DecodeAudio(const char* dataBuf, int dataSize);

    int ResampleAudio(const char* dataBuf, int dataSize);

    void ResizeAudioBufUnderMutex(char* dataBuf, int dataSize);

    void CreateVideoDecoder(int videoCodec, int width, int height, int pixFmt);

    void DecodeVideo(const char* dataBuf, int dataSize, long long timestamp);

    void DecodeAVPacket(AVPacket* avPacket, long long timestamp);

    void RecordAudio(const char* dataBuf, int dataSize,
        long long timestamp);

    void RecordVideo(const char* dataBuf, int dataSize,
        long long timestamp);

private:
    int source_id_;
    int source_type_;
    std::string source_url_;
    Observer* observer_;

    ServiceConfig service_cfg_;
public:
    SourceConfig src_cfg_;
private:
    int src_samplerate_;
    int src_channel_;

    // AAC音频解码器
    int decode_sample_rate_;
    int decode_channel_;
    std::vector<char> pcmbuf_;
    FAACDecoder* aac_decoder_;

    BufUnitHelper* unit_helper_;
    SwrContext* ff_resampler_;
    uint8_t** sample_pcmbuf_;
    int sample_pcmsize_;
    uint8_t** resample_pcmbuf_;
    int resample_max_sam_nb_;
    int dst_linesize_;

    // H.264视频解码器
    int src_width_;
    int src_height_;
    std::vector<char> rgbbuf_;
    FFVideoDecoder* h264_decoder_;
    char* extra_data_;
    int extra_size_;
    bool has_extra_used_;

    // 输入源读取
    RtmpReader* rtmp_reader_;
    FFUrlReader* rtsp_reader_;

    AudioBufferingQueue* audio_queue_;
    VideoBufferingQueue* video_queue_;

    // 时间戳
    long long audio_last_timestamp_;
    long long video_last_timestamp_;
    TimeCounter total_timer_;

    TimeCounter audio_arrive_timer_;
    TimeCounter video_arrive_timer_;

    base::Lock write_mtx_;

    // 录制
    volatile bool is_recording_;
    Recorder* recorder_;

    volatile bool is_failed_;

    bool is_audio_codec_init_;
    bool is_video_codec_init_;
};

#endif // _HBASE_SOURCE_READER_H_

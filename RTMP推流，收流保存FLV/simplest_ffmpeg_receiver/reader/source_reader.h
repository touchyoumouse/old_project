/*******************************************************************************
 * source_reader.h
 * Copyright: (c) 2014 Haibin Du(haibindev.cnblogs.com). All rights reserved.
 * -----------------------------------------------------------------------------
 *
 * ����Դ��������ӦЭ���ȡ���ݣ����н��룬�������ϼ�
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
     * ���캯��
     * @param sourceId: ����ԴID
     * @param sourceType: ����ԴЭ������
     * @param ob: �۲��ߣ��ص����������ࣩ
     * @param serviceCfg: ��������Ϣ
     * @param srcCfg: ����Դ������Ϣ
     * @returns:
     */
    explicit SourceReader(int sourceId, int sourceType, Observer* ob,
        const ServiceConfig& serviceCfg, const SourceConfig& srcCfg);

    ~SourceReader();

    /***
     * ��ʼ��ȡԴ����
     * @param requestUrl: Դ��ַ
     * @returns:
     */
    void Start(const std::string& requestUrl);

    /***
     * ֹͣ��������
     */
    void Stop();

    /***
     * ��ʼ¼��
     */
    void StartRecord();

    /***
     * ��ͣ¼��
     */
    void PauseRecord();

    /***
     * ����¼��
     */
    void StopRecord();

    /***
     * ����������Ϣ
     * @param serviceCfg: ��������Ϣ
     * @param srcCfg: ����Դ������Ϣ
     * @returns:
     */
    void UpdateConfig(const ServiceConfig& serviceCfg,
        const SourceConfig& srcCfg);

    // ����Դ�Ƿ����
    bool IsFailed() { return is_failed_; }

    // ��һ����Ƶ���ݵ����˶��
    long long AudioArriveMillsecs();

    // ��һ����Ƶ���ݵ����˶��
    long long VideoArriveMillsecs();

    // -------------------------------------------
    // base::SimpleThread �ص�����

    virtual void ThreadRun();

    // -------------------------------------------
    // FFUrlReader::Observer �ص�����

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
    // RtmpReader::Observer �ص�����

    virtual void OnRtmpErrCode(int errCode);

    virtual void OnRtmpAudioCodec(int samRate, int channel);

    virtual void OnRtmpVideoCodec(int width, int height);

    virtual void OnRtmpAudioBuf(const char* dataBuf, unsigned int dataLen,
        long long timestamp);

    virtual void OnRtmpVideoBuf(const char* dataBuf, unsigned int dataLen,
        long long timestamp);

    // -------------------------------------------
    // AudioBufferingQueue::Observer �ص�����

    virtual void OnAudioQueueData(const char* dataBuf, int dataSize);

    // -------------------------------------------
    // VideoBufferingQueue::Observer �ص�����

    virtual void OnVideoQueueData(const char* dataBuf, int dataSize,
        long long timestamp);

    /***
     * ��ʱ�������Դ�Ƿ���Ҫ����
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

    // AAC��Ƶ������
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

    // H.264��Ƶ������
    int src_width_;
    int src_height_;
    std::vector<char> rgbbuf_;
    FFVideoDecoder* h264_decoder_;
    char* extra_data_;
    int extra_size_;
    bool has_extra_used_;

    // ����Դ��ȡ
    RtmpReader* rtmp_reader_;
    FFUrlReader* rtsp_reader_;

    AudioBufferingQueue* audio_queue_;
    VideoBufferingQueue* video_queue_;

    // ʱ���
    long long audio_last_timestamp_;
    long long video_last_timestamp_;
    TimeCounter total_timer_;

    TimeCounter audio_arrive_timer_;
    TimeCounter video_arrive_timer_;

    base::Lock write_mtx_;

    // ¼��
    volatile bool is_recording_;
    Recorder* recorder_;

    volatile bool is_failed_;

    bool is_audio_codec_init_;
    bool is_video_codec_init_;
};

#endif // _HBASE_SOURCE_READER_H_

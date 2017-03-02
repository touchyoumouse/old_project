/*******************************************************************************
 * ff_url_reader.h
 * Copyright: (c) 2013 Haibin Du(haibinnet@qq.com). All rights reserved.
 * -----------------------------------------------------------------------------
 *
 * ��ȡRTSP����
 *
 * -----------------------------------------------------------------------------
 * 2014-3-10 23:29 - Created (Haibin Du)
 ******************************************************************************/

#ifndef _HBASE_FF_URL_READER_H_
#define _HBASE_FF_URL_READER_H_

#include <deque>
#include <string>

#include "../base/SimpleThread.h"
#include "../base/TimeCounter.h"

extern "C"
{
#include "libavformat/avformat.h"
}


typedef void (*FFReaderEventFunc)(void* rtsp2Rtmp, int eventCode);

class FFUrlReader : public base::SimpleThread
{
public:
    enum
    {
        FF_URL_OPEN_SUCCEED = 0,
        FF_URL_OPEN_FAILED  = 1,       // url ��ʧ��
        FF_URL_ABORT        = 2,
        FF_URL_CLOSE        = 3,
    };

    // ����rtsp���Ӻ����ݻص�
    class Observer
    {
    public:
        virtual ~Observer() {}

        // ������
        virtual void OnFFEvent(int eventId) = 0;

        // ��������RTSP��Ƶ����Ƶ������Ϣʱ
        virtual void OnFFCodecInfo(
            int audioCodec, int sampleRate, int channelCount,
            int videoCodec, int width, int height, int pixFmt,
            const char* extraBuf, int extraSize) = 0;

        // ���յ���Ƶ����
        virtual void OnFFAudioBuf(const char* dataBuf, int dataSize,
            long long timestamp, long long duration) = 0;

        // �����յ���Ƶ����
        virtual void OnFFVideoBuf(const char* dataBuf, int dataSize,
            long long timestamp, long long duration) = 0;

        // �����յ���Ƶ����
        virtual void OnFFAVPacket(AVPacket* avPacket,
            long long timestamp, long long duration) = 0;
    };

public:
    explicit FFUrlReader(Observer* ob);

    virtual ~FFUrlReader();

    virtual void ThreadRun();

    /***
     * ��ʼ��ȡRTSP����
     * @param readUrl: ����RTSP��ַ
     * @returns:
     */
    void Start(const std::string& readUrl);

    void Stop();

    std::string ReadUrl() const { return read_url_; }

    /***
     * ����ffmpeg�жϣ��Ƿ���Ҫ��ͣ����
     */
    bool IsFFNeedInterrupt();

    /***
     * �Ƿ�������Ӵ���, ����true����Ҫ������
     */
    bool IsConnectError();

private:
    // ����
    bool FFOpenStream();

    // �ر���
    void FFCloseStream();

    // ��ȡ���������
    bool FFFindCodecInfo();

    // ѭ����ȡ���������
    void FFProcessPacket();

private:
    Observer* observer_;
    std::string read_url_;

    AVFormatContext* ff_fmt_ctx_;
    volatile bool is_ff_need_interrupt_;
    int audio_stream_index_;
    int video_stream_index_;

    int width_;
    int height_;
    int sample_rate_;
    int channel_count_;

    bool is_has_audio_;
    bool is_has_video_;

    volatile bool is_opening_;
    volatile bool is_open_failed_;
    volatile bool is_reading_;

    TimeCounter timeout_watch_;
};

#endif // _HBASE_FF_URL_READER_H_

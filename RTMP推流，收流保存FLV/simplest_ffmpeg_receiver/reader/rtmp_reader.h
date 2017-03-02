/*******************************************************************************
 * rtmp_reader.h
 * Copyright: (c) 2014 Haibin Du(haibindev.cnblogs.com). All rights reserved.
 * -----------------------------------------------------------------------------
 *
 * Base on RtmpDownloader
 *
 * -----------------------------------------------------------------------------
 * 2014-12-11 16:37 - Created (Haibin Du)
 ******************************************************************************/

#ifndef _HDEV_RTMP_READER_H_
#define _HDEV_RTMP_READER_H_

#include "../base/base.h"
#include "../base/SimpleThread.h"

#include <string>
#include <vector>

#include "../base/TimeCounter.h"
#include "../rtmp_base/librtmp/rtmp.h"

class RtmpReader : public base::SimpleThread
{
public:
    // rtmp���ش�����
    enum RtmpDErrCode
    {
        kRtmpInit,
        kRtmpConnecting,        // ��������
        kRtmpConnectSuccess,    // ���ӳɹ�
        kRtmpConnectFailed,     // ����ʧ��
        kRtmpRecvFailed,        // ���ݽ���ʧ��
        kRtmpCodecUnSupported,  // ���벻֧��
    };

    // ����rtmp����״�������ݻص�
    class Observer
    {
    public:
        virtual ~Observer() {}

        // ������
        virtual void OnRtmpErrCode(int errCode) = 0;

        // ��������AAC�����ʺ�����ʱ
        virtual void OnRtmpAudioCodec(int samRate, int channel) = 0;

        // ��������AVC�ֱ���ʱ
        virtual void OnRtmpVideoCodec(int width, int height) = 0;

        // �����յ���Ƶ����
        virtual void OnRtmpVideoBuf(const char* dataBuf, unsigned int dataLen,
            long long timestamp) = 0;

        // ���յ���Ƶ����
        virtual void OnRtmpAudioBuf(const char* dataBuf, unsigned int dataLen,
            long long timestamp) = 0;
    };

public:
    /***
     * ���캯��������librtmp����������Ҫ������־�ļ�("librtmp.log")
     * @param isNeedLog: �Ƿ���librtmp��־
     * @returns:
     */
    RtmpReader(bool isNeedLog);

    /***
     * ������������RtmpDownloader����֮ǰ����Ҫ����Stop��������ֹͣ���ز��ͷ���Դ
     */
    ~RtmpReader();

    /***
     * ����Url�����������̣߳���������ʱ�������߳��ѿ�����ע�⣬�����������������߳��н��У�
     * ��ˣ��˺�������ture�����������ӳɹ���
     * @param url: rtmp url�����ַ���
     * @returns: false����url����
     */
    bool Start(const char* url);

    /***
     * �ر�RtmpDownloader�����ȴ��߳̽���
     */
    void Stop();

    /***
     * ���ü����ص���
     */
    void SetObserver(Observer* ob);

    bool IsNeedReConnected();

    /***
     * �̳���base::SimpleThread
     */
    virtual void ThreadRun();

    int Width() { return video_width_; }

    int Height() { return video_height_; }

    int Samplerate() { return samplerate_; }

    int Channel() { return channel_; }

    bool IsCodecError() { return is_codec_error_; }

private:
    // ��������
    bool Connect(const char* url);

    // �ر����Ӻ�librtmp���ͷ���Դ
    void Close();

    void ParseVideoPacket(const char* dataBuf, unsigned int dataLen);

    void ReadH264SequenceHeader(const char* pbuf, unsigned int buflen);

    void ReadH264Nalus(const char* pbuf, unsigned int buflen);

    void ParseAudioPacket(const char* dataBuf, unsigned int dataLen);

    void ReadAACSequenceHeader(const char* pbuf, unsigned int buflen);

    void ReadAACPackets(const char* pbuf, unsigned int buflen);

    void ParseFlashTag(const char* dataBuf, unsigned int dataLen);

private:
    RTMP* rtmp_;
    char* streming_url_;
    FILE* flog_;
    Observer* observer_;

    volatile bool is_stopping_;
    RtmpDErrCode connect_state_;

    int video_width_;
    int video_height_;
    bool is_get_keyframe_;

    int samplerate_;
    int channel_;
    bool is_codec_callback_;

    volatile bool is_codec_error_;

    TimeCounter recv_timecounter_;
    long long audio_timestamp_;
    long long video_timestamp_;
};

#endif // _HDEV_RTMP_READER_H_

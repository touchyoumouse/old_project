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
    // rtmp下载错误码
    enum RtmpDErrCode
    {
        kRtmpInit,
        kRtmpConnecting,        // 正在连接
        kRtmpConnectSuccess,    // 连接成功
        kRtmpConnectFailed,     // 连接失败
        kRtmpRecvFailed,        // 数据接收失败
        kRtmpCodecUnSupported,  // 编码不支持
    };

    // 监听rtmp连接状况和数据回调
    class Observer
    {
    public:
        virtual ~Observer() {}

        // 错误码
        virtual void OnRtmpErrCode(int errCode) = 0;

        // 当解析到AAC采样率和声道时
        virtual void OnRtmpAudioCodec(int samRate, int channel) = 0;

        // 当解析到AVC分辨率时
        virtual void OnRtmpVideoCodec(int width, int height) = 0;

        // 当接收到视频数据
        virtual void OnRtmpVideoBuf(const char* dataBuf, unsigned int dataLen,
            long long timestamp) = 0;

        // 接收到音频数据
        virtual void OnRtmpAudioBuf(const char* dataBuf, unsigned int dataLen,
            long long timestamp) = 0;
    };

public:
    /***
     * 构造函数，创建librtmp，并根据需要设置日志文件("librtmp.log")
     * @param isNeedLog: 是否开启librtmp日志
     * @returns:
     */
    RtmpReader(bool isNeedLog);

    /***
     * 析构函数，在RtmpDownloader析构之前，需要调用Stop函数，来停止下载并释放资源
     */
    ~RtmpReader();

    /***
     * 解析Url并开启下载线程，函数返回时，下载线程已开启。注意，发起连接是在下载线程中进行，
     * 因此，此函数返回ture并不表明连接成功。
     * @param url: rtmp url完整字符串
     * @returns: false表明url错误
     */
    bool Start(const char* url);

    /***
     * 关闭RtmpDownloader，并等待线程结束
     */
    void Stop();

    /***
     * 设置监听回调类
     */
    void SetObserver(Observer* ob);

    bool IsNeedReConnected();

    /***
     * 继承自base::SimpleThread
     */
    virtual void ThreadRun();

    int Width() { return video_width_; }

    int Height() { return video_height_; }

    int Samplerate() { return samplerate_; }

    int Channel() { return channel_; }

    bool IsCodecError() { return is_codec_error_; }

private:
    // 发起连接
    bool Connect(const char* url);

    // 关闭连接和librtmp，释放资源
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

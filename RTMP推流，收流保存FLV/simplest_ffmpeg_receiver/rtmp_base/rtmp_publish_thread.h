/*******************************************************************************
 * rtmp_publish_thread.h
 * Copyright: (c) 2014 Haibin Du(haibindev.cnblogs.com). All rights reserved.
 * -----------------------------------------------------------------------------
 *
 * rtmp�������̣߳���װRtmpPublish���Դ��̣߳����Զ�����
 * ������rtmp_synth_service��Ŀ���RtmpPublisher�汾�ϸĶ�
 *
 * -----------------------------------------------------------------------------
 * 2015-6-5 9:19 - Created (Haibin Du)
 ******************************************************************************/

#ifndef _HDEV_RTMP_PUBLISH_THREAD_H_
#define _HDEV_RTMP_PUBLISH_THREAD_H_

#include "base/base.h"

#include <deque>
#include <string>

#include "base/Lock.h"
#include "base/SimpleThread.h"
#include "base/TimeCounter.h"

struct RtmpPubAudio
{
    char* audio_buf_;
    int audio_size_;
    long long timestamp_;

    RtmpPubAudio()
    {
        audio_buf_ = NULL;
        audio_size_ = 0;
        timestamp_ = -1;
    }

    ~RtmpPubAudio()
    {
        delete[] audio_buf_;
    }
};

class RtmpPublish;
class RtmpPublishThread : public base::SimpleThread
{
public:
    RtmpPublishThread(
        const std::string& rtmpUrl, // ����rtmp���ĵ�ַ(rtmp://x.x.x.x/live/streamname)
        bool isNeedLog,             // �Ƿ���Ҫlibrtmp��ӡ��־
        int audioSampleRate,        // ��Ƶ������(һ��Ϊ44100 22050 16000)
        int audioChannels,          // ��Ƶ������(һ��Ϊ1 2)
        int videoWidth,             // ��Ƶ��
        int videoHeight             // ��Ƶ��
        );

    ~RtmpPublishThread();

    virtual void ThreadRun();

    void Start();

    void Stop();

    // ����

    void PostAACFrame(char* buf, int bufLen);

    void PostH264Frame(char* buf, int bufLen, long long shouldTS);

    bool IsConnectFailed();

    void OnAACTimerCallback();

private:
    RtmpPublish* rtmp_pub_;
    std::string rtmp_url_;
    bool is_need_log_;

    int source_samrate_;
    int source_channel_;
    int video_width_;
    int video_height_;

    base::Lock queue_mtx_;
    std::deque<RtmpPubAudio*> audio_queue_;
    long long queue_audio_frame_count_;
    TimeCounter time_counter_;
    MMRESULT timer_id_;

    volatile bool is_restarting_;
};

#endif // _HDEV_RTMP_PUBLISH_THREAD_H_

/*******************************************************************************
 * rtmp_publish_thread.cpp
 * Copyright: (c) 2014 Haibin Du(haibindev.cnblogs.com). All rights reserved.
 * -----------------------------------------------------------------------------
 *
 * -----------------------------------------------------------------------------
 * 2015-6-5 9:21 - Created (Haibin Du)
 ******************************************************************************/

#include "rtmp_publish_thread.h"

#if defined(WIN32)
#include <ObjBase.h>
#include <Shlwapi.h>
#endif

#include "base/simple_logger.h"
#include "rtmp_publish.h"

#define USE_TIMER_EVENT

RtmpPublishThread::RtmpPublishThread(
    const std::string& rtmpUrl, // 发布rtmp流的地址(rtmp://x.x.x.x/live/streamname)
    bool isNeedLog,             // 是否需要librtmp打印日志
    int audioSampleRate,        // 音频采样率(一般为44100 22050 16000)
    int audioChannels,          // 音频声道数(一般为1 2)
    int videoWidth,             // 视频宽
    int videoHeight             // 视频高
    )
{
    rtmp_pub_ = NULL;

    rtmp_url_ = rtmpUrl;
    is_need_log_ = isNeedLog;

    source_samrate_ = audioSampleRate;
    source_channel_ = audioChannels;
    video_width_ = videoWidth;
    video_height_ = videoHeight;

    queue_audio_frame_count_ = 0;
    timer_id_ = 0;

    is_restarting_ = false;
}

RtmpPublishThread::~RtmpPublishThread()
{
    delete rtmp_pub_;
}

void RtmpPublishThread::Start()
{
    ThreadStart();
}

void RtmpPublishThread::Stop()
{
    if (timer_id_)
    {
        timeKillEvent(timer_id_);
        timer_id_ = 0;
    }

    ThreadStop();
    ThreadJoin();

    rtmp_pub_->Stop();
}

void RtmpPublishThread::ThreadRun()
{
#ifdef WIN32
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    WORD version;  
    WSADATA wsaData;  
    version = MAKEWORD(1, 1);
    WSAStartup(version, &wsaData);
#endif

    rtmp_pub_ = new RtmpPublish(rtmp_url_, is_need_log_,
        source_samrate_, source_channel_,
        video_width_, video_height_);
    rtmp_pub_->Start();

    long long audio_frame_count = 0;

    while (true)
    {
        if (base::SimpleThread::IsThreadStop()) break;

        if (rtmp_pub_)
        {
            if (rtmp_pub_->IsNeedReConnect())
            {
                SIMPLE_LOG("rtmp pub need reconnect: %s\n", rtmp_url_.c_str());

                is_restarting_ = true;

                if (timer_id_)
                {
                    timeKillEvent(timer_id_);
                    timer_id_ = 0;
                }

                RtmpPublish* oldpub = rtmp_pub_;
                rtmp_pub_ = NULL;

                oldpub->Stop();
                delete oldpub;

                oldpub = new RtmpPublish(rtmp_url_, is_need_log_,
                    source_samrate_, source_channel_,
                    video_width_, video_height_);
                oldpub->Start();

                while (true)
                {
                    if (oldpub->GetPubState() != RtmpPublish::RTMP_PUB_INIT &&
                        oldpub->GetPubState() != RtmpPublish::RTMP_PUB_CONNECTING)
                    {
                        break;
                    }
                    MillsecSleep(500);
                }
                rtmp_pub_ = oldpub;

                is_restarting_ = false;
            }
#if !defined(USE_TIMER_EVENT)
            else
            {
                long long should_timestamp = audio_frame_count*1000/source_samrate_+100;

                if (time_counter_.Get() >= should_timestamp)
                {
                    std::deque<RtmpPubAudio*> audio_bufs;
                    {
                        base::AutoLock lg(queue_mtx_);

                        if (false == audio_queue_.empty())
                        {
                            RtmpPubAudio* audio_buf = audio_queue_.front();
                            audio_queue_.pop_front();

                            audio_bufs.push_back(audio_buf);

                            audio_frame_count += 1024;
                        }
                    }

                    while (false == audio_bufs.empty())
                    {
                        RtmpPubAudio* audio_buf = audio_bufs.front();
                        audio_bufs.pop_front();

                        rtmp_pub_->PostAACFrame(audio_buf->audio_buf_, audio_buf->audio_size_);

                        delete audio_buf;
                    }
                }
            }
#endif
        }

        MillsecSleep(1);
    }

#ifdef WIN32
    WSACleanup();

    CoUninitialize();
#endif
}

static void CALLBACK AACTimerCallback(UINT uID, UINT uMsg,
    DWORD_PTR dwUser, DWORD dw1, DWORD dw2)
{
    RtmpPublishThread* pub_thread = (RtmpPublishThread*)dwUser;
    if (pub_thread)
    {
        pub_thread->OnAACTimerCallback();
    }
}

void RtmpPublishThread::OnAACTimerCallback()
{
    if (time_counter_.Get() >= 100)
    {
        RtmpPubAudio* audio_buf = NULL;

        {
            base::AutoLock lg(queue_mtx_);

            if (false == audio_queue_.empty())
            {
                audio_buf = audio_queue_.front();
                audio_queue_.pop_front();
            }

            SIMPLE_LOG("queue len: %d\n", audio_queue_.size());
        }

        if (rtmp_pub_ && audio_buf)
        {
            SIMPLE_LOG("audio buf timestamp: %lld, timer: %lld\n", audio_buf->timestamp_, time_counter_.Get());

            rtmp_pub_->PostAACFrame(audio_buf->audio_buf_, audio_buf->audio_size_);

            delete audio_buf;
        }
    }
}

void RtmpPublishThread::PostAACFrame(char* buf, int bufLen)
{
    if (SimpleThread::IsThreadStop()) return;

    if (is_restarting_) return;

    if (time_counter_.LastTimestamp() == -1)
    {
        time_counter_.Reset();
    }

#if defined(USE_TIMER_EVENT)
    if (timer_id_ == 0)
    {
        SIMPLE_LOG("\n");

        int wakeup_interval = 1000 * 1024 / source_samrate_;
        int resolution = wakeup_interval / 4;;
        timer_id_ = ::timeSetEvent(wakeup_interval, resolution, AACTimerCallback,
            (DWORD_PTR)this, TIME_PERIODIC | TIME_KILL_SYNCHRONOUS);
    }
#endif

#if 1
    RtmpPubAudio* pub_audio = new RtmpPubAudio;
    pub_audio->audio_buf_ = new char[bufLen];
    memcpy(pub_audio->audio_buf_, buf, bufLen);
    pub_audio->audio_size_ = bufLen;
    pub_audio->timestamp_ = queue_audio_frame_count_*1000/source_samrate_;
    queue_audio_frame_count_+= 1024;

    base::AutoLock al(queue_mtx_);
    audio_queue_.push_back(pub_audio);
#else
     if (rtmp_pub_)
     {
         rtmp_pub_->PostAACFrame(buf, bufLen);
     }
#endif
}

void RtmpPublishThread::PostH264Frame(char* buf, int bufLen, long long shouldTS)
{
    if (SimpleThread::IsThreadStop()) return;

    if (is_restarting_) return;
    if (rtmp_pub_)
    {
        rtmp_pub_->PostH264Frame(buf, bufLen, shouldTS);
    }
}

bool RtmpPublishThread::IsConnectFailed()
{
    if (rtmp_pub_)
    {
        return rtmp_pub_->GetPubState() == 
            RtmpPublish::RTMP_PUB_CONNECT_FAILED;
    }
    return false;
}

#include "stdafx.h"
#include "RtmpLiveScreenCmd.h"

RtmpLiveScreenCmd::RtmpLiveScreenCmd()
{
    // 音视频设备

    std::map<CString, CString> a_devices, v_devices;
    DSCapture::ListAudioCapDevices(a_devices);
    DSCapture::ListVideoCapDevices(v_devices);

    for (std::map<CString, CString>::iterator it = a_devices.begin();
        it != a_devices.end(); ++it)
    {
        audio_device_index_.push_back(it->first);
    }

    for (std::map<CString, CString>::iterator it = v_devices.begin();
        it != v_devices.end(); ++it)
    {
        video_device_index_.push_back(it->first);
    }

    // 屏幕分辨率
    HDC hscreen = ::GetDC(NULL);
    screen_width_ = ::GetDeviceCaps(hscreen, HORZRES);
    screen_height_ = ::GetDeviceCaps(hscreen, VERTRES);
    ::DeleteDC(hscreen);

    rtmp_live_screen_ = 0;
}

RtmpLiveScreenCmd::~RtmpLiveScreenCmd()
{
    if (rtmp_live_screen_)
    {
        rtmp_live_screen_->Stop();
        rtmp_live_screen_->Join();
        delete rtmp_live_screen_;
    }
}

void RtmpLiveScreenCmd::LoadConfig(const std::string& cfgFile)
{
    config_.Load("livescreen_config.txt");

    is_cap_webcam_ = config_.GetInt("is_capure_webcam", 0) ? true: false;
    webcam_x_ = config_.GetInt("webcam_x", 0);
    webcam_y_ = config_.GetInt("webcam_y", 0);
    webcam_width_ = config_.GetInt("webcam_width", 320);
    webcam_height_ = config_.GetInt("webcam_height", 240);

    is_cap_screen_ = config_.GetInt("is_cap_screen", 1) ? true: false;
    is_need_record_ = config_.GetInt("is_need_record", 0) ? true: false;

    rtmp_url_ = config_.GetProperty("rtmpurl");
    screen_bitrate_ = config_.GetInt("bitrate", 300);
    webcam_bitrate_ = config_.GetInt("webcam_bitrate", 80);
}

void RtmpLiveScreenCmd::Start()
{
    CString audio_device_id = _T("");
    if (audio_device_index_.size())
    {
        audio_device_id = audio_device_index_[0];
    }
    CString video_device_id = _T("");
    if (video_device_index_.size() && is_cap_webcam_)
    {
        video_device_id = video_device_index_[0];
    }
    else
    {
        is_cap_webcam_ = false;
    }

    rtmp_live_screen_ = new RtmpLiveScreen(audio_device_id, video_device_id, webcam_width_, webcam_height_, 
        NULL, 0, 0, rtmp_url_,
        is_cap_screen_, is_need_record_, webcam_x_, webcam_y_, webcam_width_, webcam_height_,
        webcam_bitrate_, screen_bitrate_, 3000,0);
    rtmp_live_screen_->SetCaptureWebcam(is_cap_webcam_);
    rtmp_live_screen_->Start();
}

void RtmpLiveScreenCmd::Stop()
{
    rtmp_live_screen_->Stop();
    rtmp_live_screen_->Join();
    delete rtmp_live_screen_;
    rtmp_live_screen_ = 0;
}

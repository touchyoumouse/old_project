#pragma once

#include <string>
#include <vector>

#include "RtmpLiveScreen.h"
#include "SimpleConfig.h"

class RtmpLiveScreenCmd
{
public:
    RtmpLiveScreenCmd();

    ~RtmpLiveScreenCmd();

    void LoadConfig(const std::string& cfgFile);

    void Start();

    void Stop();

private:
    std::vector<CString> audio_device_index_;
    std::vector<CString> video_device_index_;

    bool is_cap_webcam_;
    int webcam_x_;
    int webcam_y_;
    int webcam_width_;
    int webcam_height_;

    bool is_cap_screen_;
    bool is_need_record_;

    std::string rtmp_url_;
    int screen_bitrate_;
    int webcam_bitrate_;

    int screen_width_;
    int screen_height_;

    SimpleConfig config_;
    RtmpLiveScreen* rtmp_live_screen_;
};

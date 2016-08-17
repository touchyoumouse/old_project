#ifndef _RTMP_LIVE_ENCODER_H_
#define _RTMP_LIVE_ENCODER_H_

#include <deque>
#include <string>

#include "base/DataBuffer.h"
#include "base/Lock.h"
#include "base/SimpleThread.h"

#include "dshow/DSCapture.h"

// 获取当前ppt页码
#include <ObjBase.h>
#include "CpCount.h"

struct RtmpDataBuffer
{
    int type;
    base::DataBuffer* data;
    unsigned int timestamp;
    bool is_keyframe;

    RtmpDataBuffer(int type_i, base::DataBuffer* data_i, unsigned int ts_i, bool iskey_i)
    {
        type = type_i;
        data = data_i;
        timestamp = ts_i;
        is_keyframe = iskey_i;
    }

    RtmpDataBuffer()
    {
        type = -1;
        data = NULL;
        timestamp = 0;
        is_keyframe = false;
    }
};

enum SystemStateType
{
    STATE_WORKING,
    STATE_IDLE,
};

class LibRtmp;
class RtmpLiveScreen
    : public base::SimpleThread
    , public DSCaptureListener
{
public:
    RtmpLiveScreen(const CString& audioDeviceID, const CString& videoDeviceID,
        int width, int height, OAHWND owner, int previewWidth, int previewHeight,
        const std::string& rtmpUrl, bool isNeedScreen, bool isNeedRecord,
        int capX, int capY, int capWidth, int capHeight,
        int webcamBitrate, int screenBitrate, int videoChangeMillsecs,int waitTime);

    ~RtmpLiveScreen();

    virtual void Run();

    virtual void OnCaptureAudioBuffer(base::DataBuffer* dataBuf, unsigned int timestamp);

    virtual void OnCaptureVideoBuffer(base::DataBuffer* dataBuf, unsigned int timestamp, bool isKeyframe);

    void PostBuffer(base::DataBuffer* dataBuf);

    virtual void OnSPSAndPPS(char* spsBuf, int spsSize, char* ppsBuf, int ppsSize);

    void SetCaptureWebcam(bool isCapture);

    void SetWebcamPosition(int x, int y);

    int GetCurrentPPTPCount();

    bool IsNeedReConnect();

private:
    bool SendVideoDataPacket(base::DataBuffer* dataBuf, unsigned int timestamp, bool isKeyframe);

    bool SendAudioDataPacket(base::DataBuffer* dataBuf, unsigned int timestamp);

    void SendMetadataPacket();

    void SendAVCSequenceHeaderPacket();

    void SendAACSequenceHeaderPacket();

    char* WriteMetadata(char* buf);

    char* WriteAVCSequenceHeader(char* buf);

    char* WriteAACSequenceHeader(char* buf);

    unsigned int GetTimestamp();

    unsigned int GetInputTimestamp();

private:
	int wait_time_;
    DSCapture* ds_capture_;
    LibRtmp* librtmp_;
    std::string rtmp_url_;
    bool has_audio_;

    // metadata
    int width_;
    int height_;

    char* sps_;        // sequence parameter set
    int sps_size_;
    char* pps_;        // picture parameter set
    int pps_size_;

    __int64 time_begin_;
    __int64 last_timestamp_;

    std::deque<RtmpDataBuffer> process_buf_queue_;
    base::Lock queue_lock_;

    CpCount* ppt_fetcher_;
    int last_page_count_;

    unsigned int last_input_timestamp_;
    unsigned int video_change_millsecs_;
    SystemStateType state_;

    volatile bool is_send_failed_;

};

#endif // _RTMP_LIVE_ENCODER_H_

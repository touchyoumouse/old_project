#ifndef _VIDEO_ENCODER_THREAD_H_
#define _VIDEO_ENCODER_THREAD_H_

#include <string>

#include "base/SimpleThread.h"

class DSCaptureListener;
class X264Encoder;
class DSVideoGraph;
class VideoEncoderThread : public base::SimpleThread
{
public:
    VideoEncoderThread(DSVideoGraph* dsVideoGraph, int webcamBitrate, int screenBitrate);

    ~VideoEncoderThread();

    virtual void Run();

    void SetCaptureWH(int capX, int capY, int capWidth, int capHeight);

    void SetListener(DSCaptureListener* listener) { listener_ = listener; }

    void SetOutputFilename(const std::string& filename);

    void SavePicture() { is_save_picture_ = true; }

    void SetCaptureWebcam(bool isCapture);

    void SetCaptureScreen(bool isCapture);

    void SetWebcamPosition(int x, int y) { capx1_ = x; capy1_ = y; }

private:
    bool RGB2YUV420(LPBYTE RgbBuf,UINT nWidth,UINT nHeight,LPBYTE yuvBuf,unsigned long *len);

    void SaveRgb2Bmp(char* rgbbuf, unsigned int width, unsigned int height);

private:
    DSVideoGraph* ds_video_graph_;
    X264Encoder* x264_encoder_;
    std::string filename_264_;
    bool is_save_picture_;
    DSCaptureListener* listener_;
    int capx1_, capy1_;
    int cap_width;
    int cap_height_;
    int screen_width_;
    int screen_height_;
    bool is_capture_webcam_;
    bool is_capture_screen_;
    int webcam_bitrate_;
    int screen_bitrate_;
};

#endif // _VIDEO_ENCODER_THREAD_H_

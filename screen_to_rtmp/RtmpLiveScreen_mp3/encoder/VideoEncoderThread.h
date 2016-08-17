#ifndef _VIDEO_ENCODER_THREAD_H_
#define _VIDEO_ENCODER_THREAD_H_

#include <string>

#include "base/SimpleThread.h"

class DSCaptureListener;
class X264Encoder;
class DSVideoGraph;
class LiveImage;

class VideoEncoderThread : public base::SimpleThread
{
public:
    VideoEncoderThread(DSVideoGraph* dsVideoGraph, int webcamBitrate, int screenBitrate);

    ~VideoEncoderThread();

    virtual void Run();

    void SetListener(DSCaptureListener* listener) { listener_ = listener; }

    void SavePicture() { is_save_picture_ = true; }

    void SetCaptureWebcam(bool isCapture);

    void SetCaptureScreen(bool isCapture);

    void SetWebcamPosition(int x, int y) { webcam_x_ = x; webcam_y_ = y; }

    void SetCaptureWH(int cropX, int cropY, int cropWidth, int cropHeight)
    {
        webcam_x_ = cropX; webcam_y_ = cropY;
        //crop_width_ = cropWidth;
        //crop_height_ = cropHeight;
    }

private:
    bool RGB2YUV420(LPBYTE RgbBuf,UINT nWidth,UINT nHeight,LPBYTE yuvBuf,unsigned long *len);

    bool RGB2YUV420_r(LPBYTE RgbBuf,UINT nWidth,UINT nHeight,LPBYTE yuvBuf,unsigned long *len);

    void GetDesktopImage(char* outRgbbuf);

private:
    DSVideoGraph* ds_video_graph_;
    X264Encoder* x264_encoder_;
    bool is_save_picture_;
    DSCaptureListener* listener_;

    LiveImage* live_image_;
    int encoder_width_;
    int encoder_height_;

    int webcam_x_, webcam_y_;

    // screen
    int screen_width_;
    int screen_height_;
    int screen_chroma_;
    int screen_pix_bytes_;
    int screen_buf_size_;
    int cropx_;
    int cropy_;
    int crop_width_;
    int crop_height_;

    bool is_capture_webcam_;
    bool is_capture_screen_;

    int webcam_bitrate_;
    int screen_bitrate_;
};

#endif // _VIDEO_ENCODER_THREAD_H_

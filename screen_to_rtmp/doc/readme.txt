void RtmpLiveScreenCmd::Start() ——> void SimpleThread::Start() ——>

DWORD __stdcall ThreadFunc(void* params)——>thread->ThreadMain();——>


//启动摄像头
is_capture_webcam_ = ds_video_graph_->IsCreateOK();

音频采集
void AudioEncoderThread::Run()

//截屏
void RtmpLiveScreenDlg::OnBnClickedBtnStart() ——> 
    video_encoder_thread_ = new VideoEncoderThread(ds_video_graph_,webcamBitrate, screenBitrate);
 ——> 
VideoEncoderThread::VideoEncoderThread(DSVideoGraph* dsVideoGraph, int webcamBitrate, int screenBitrate)
    : ds_video_graph_(dsVideoGraph), x264_encoder_(new X264Encoder)
    , is_save_picture_(false), webcam_bitrate_(webcamBitrate), screen_bitrate_(screenBitrate)

 ——> 
screen_width_ = ::GetDeviceCaps(hscreen, HORZRES);

 ——> 

void VideoEncoderThread::Run()
｛
GetDesktopImage(rgbbuf, capx1_, capy1_, ds_video_graph_->Width(),  ds_video_graph_->Height(), tmpbuf);
RGB2YUV420((LPBYTE)tmpbuf, screen_width_, screen_height_, (LPBYTE)yuvbuf,  &yuvimg_size);
x264buf_len = yuvimg_size*100;
x264_encoder_->Encode(yuvbuf, x264buf, x264buf_len, is_keyframe);
｝







邓心潼 邓伶琳 邓雅伦 邓丽絮 邓沛玲 
邓韵心 邓心欣 邓婧雯 邓笑虹 邓诗嘉 

邓润燕 邓心诺 邓伶琳 邓嘉玲 邓宝玲 
邓名华 邓嫣红 邓傲菡 邓依蕊 邓佳瞳 

邓安邦 邓立辉 邓志明 邓光耀 邓阳晖 
邓温茂 邓文石 邓展鹏 邓国兴 邓阳夏 



void RtmpLiveScreenCmd::Start() ����> void SimpleThread::Start() ����>

DWORD __stdcall ThreadFunc(void* params)����>thread->ThreadMain();����>


//��������ͷ
is_capture_webcam_ = ds_video_graph_->IsCreateOK();

��Ƶ�ɼ�
void AudioEncoderThread::Run()

//����
void RtmpLiveScreenDlg::OnBnClickedBtnStart() ����> 
    video_encoder_thread_ = new VideoEncoderThread(ds_video_graph_,webcamBitrate, screenBitrate);
 ����> 
VideoEncoderThread::VideoEncoderThread(DSVideoGraph* dsVideoGraph, int webcamBitrate, int screenBitrate)
    : ds_video_graph_(dsVideoGraph), x264_encoder_(new X264Encoder)
    , is_save_picture_(false), webcam_bitrate_(webcamBitrate), screen_bitrate_(screenBitrate)

 ����> 
screen_width_ = ::GetDeviceCaps(hscreen, HORZRES);

 ����> 

void VideoEncoderThread::Run()
��
GetDesktopImage(rgbbuf, capx1_, capy1_, ds_video_graph_->Width(),  ds_video_graph_->Height(), tmpbuf);
RGB2YUV420((LPBYTE)tmpbuf, screen_width_, screen_height_, (LPBYTE)yuvbuf,  &yuvimg_size);
x264buf_len = yuvimg_size*100;
x264_encoder_->Encode(yuvbuf, x264buf, x264buf_len, is_keyframe);
��







������ ������ ������ ������ ������ 
������ ������ ����� ��Ц�� ��ʫ�� 

������ ����ŵ ������ �˼��� �˱��� 
������ ���̺� �˰��� ������ �˼�ͫ 

�˰��� ������ ��־�� �˹�ҫ ������ 
����ï ����ʯ ��չ�� �˹��� ������ 



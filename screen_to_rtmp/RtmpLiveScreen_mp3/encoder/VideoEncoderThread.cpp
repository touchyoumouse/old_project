#include "stdafx.h"
#include "VideoEncoderThread.h"

#include "dshow/DSCapture.h"
#include "dshow/DSVideoGraph.h"
#include "X264Encoder.h"

VideoEncoderThread::VideoEncoderThread(DSVideoGraph* dsVideoGraph, int webcamBitrate, int screenBitrate)
    : ds_video_graph_(dsVideoGraph), x264_encoder_(new X264Encoder)
    , is_save_picture_(false), webcam_bitrate_(webcamBitrate), screen_bitrate_(screenBitrate)
{
    capx1_ = 0;
    capy1_ = 0;
    cap_width = 0;
    cap_height_ = 0;

    HDC hscreen = ::GetDC(NULL);
    screen_width_ = ::GetDeviceCaps(hscreen, HORZRES);
    screen_height_ = ::GetDeviceCaps(hscreen, VERTRES);

    listener_ = NULL;
    is_capture_webcam_ = true;
    is_capture_screen_ = true;
}

VideoEncoderThread::~VideoEncoderThread()
{
    delete x264_encoder_;
}

void VideoEncoderThread::SetCaptureWH(int capX, int capY, int capWidth, int capHeight)
{
    capx1_ = capX;
    capy1_ = capY;
    cap_width = capWidth;
    cap_height_ = capHeight;
}

void GetDesktopImage(char* capRgbbuf, int capX1, int capY1, int capWidth, int capHeight,
    char* outRgbbuf)
{
    HDC         hScrDC, hMemDC;
    int         nWidth, nHeight;
    int         xScrn, yScrn;
    HGDIOBJ     hOldBitmap , hBitmap;

    // create a DC for the screen and create     
    // a memory DC compatible to screen DC    
    //hScrDC = CreateDC(_T("DISPLAY"), NULL, NULL, NULL);     
    hScrDC = GetDC(NULL);
    hMemDC = CreateCompatibleDC(hScrDC);      // get points of rectangle to grab 

    xScrn = GetDeviceCaps(hScrDC, HORZRES);     
    yScrn = GetDeviceCaps(hScrDC, VERTRES);     

    // create a bitmap compatible with the screen DC     
    hBitmap = CreateCompatibleBitmap(hScrDC, xScrn, yScrn);      

    // select new bitmap into memory DC     
    hOldBitmap =   SelectObject (hMemDC, hBitmap);      

    nWidth = xScrn;
    nHeight = yScrn;

    // bitblt screen DC to memory DC     
    BitBlt(hMemDC, 0, 0, nWidth, nHeight, hScrDC, 0, 0, SRCCOPY);
    //BitBlt(hMemDC, capX, capY, capWidth, capHeight, );

    // select old bitmap back into memory DC and get handle to     
    // bitmap of the screen          
    hBitmap = SelectObject(hMemDC, hOldBitmap);      


    BITMAPINFO bmp_info_;
    memset( &bmp_info_, 0, sizeof(bmp_info_) );
    bmp_info_.bmiHeader.biSize = sizeof( BITMAPINFOHEADER );
    bmp_info_.bmiHeader.biWidth = nWidth;
    bmp_info_.bmiHeader.biHeight = -nHeight;
    bmp_info_.bmiHeader.biPlanes = 1;
    bmp_info_.bmiHeader.biBitCount = 24;
    bmp_info_.bmiHeader.biCompression = BI_RGB;
    bmp_info_.bmiHeader.biSizeImage = nWidth*nHeight*3;

    int ret = GetDIBits(hMemDC, (HBITMAP)hBitmap, 0, nHeight, outRgbbuf, &bmp_info_, DIB_RGB_COLORS);

    if (capRgbbuf)
    {
        int capX2 = capX1 + capWidth;
        int capY2 = capY1 + capHeight;
        int cappos = 0;
        for (int yi = capY1; yi < capY2; yi++)
        {
            char* pbuf = outRgbbuf + xScrn * (yScrn - 1 - yi) * 3 + capX1*3; 
            for (int xi = capX1; xi < capX2; xi++)
            {
                *(pbuf++) = capRgbbuf[cappos++];
                *(pbuf++) = capRgbbuf[cappos++];
                *(pbuf++) = capRgbbuf[cappos++];
            }
        }
    }

    //hBitmap = CreateDIBSection(hMemDC, &bmp_info_, DIB_RGB_COLORS, (void**)&rgbbuf, NULL, 0);

    BITMAPFILEHEADER bmpHeader;
    ZeroMemory(&bmpHeader,sizeof(BITMAPFILEHEADER));
    bmpHeader.bfType = 0x4D42;
    bmpHeader.bfOffBits = sizeof(BITMAPINFOHEADER)+sizeof(BITMAPFILEHEADER);
    bmpHeader.bfSize = bmpHeader.bfOffBits + nWidth*nHeight*3;

    FILE* fp = fopen("./CameraCodingCapture.bmp", "wb");
    if (0)
    {
        fwrite(&bmpHeader, 1, sizeof(BITMAPFILEHEADER), fp);
        fwrite(&(bmp_info_.bmiHeader), 1, sizeof(BITMAPINFOHEADER), fp);
        fwrite(outRgbbuf, 1, nWidth*nHeight*3, fp);
        fclose(fp);
    }

    // clean up      
    DeleteDC(hScrDC);     
    DeleteDC(hMemDC);
    DeleteObject(hBitmap);
    DeleteObject(hOldBitmap);
}

void VideoEncoderThread::Run()
{
    FILE* fp_264 = 0;
    FILE* fp_yuv = 0;
    FILE* fp_log = 0;
    if (false == filename_264_.empty())
    {
        fp_264 = fopen(filename_264_.c_str(), "wb");//wb
		//BYTE byteshead[4];
		//byteshead[0]=0x00;
		//byteshead[1]=0x00;
		//byteshead[2]=0x00;
		//byteshead[3]=0x01;
		//fwrite(byteshead, 4, 1, fp_264);
    }

	//----------------------------
    fp_yuv = fopen("d:\\yuy420.yuv", "wb");
	//----------------------------

	
    int capture_width;
    int capture_height;
    if (is_capture_screen_)
    {
        capture_width = screen_width_;
        capture_height = screen_height_;
		//screen_bitrate_=3000;
        x264_encoder_->Initialize(capture_width, capture_height, screen_bitrate_, 25); 
    }
    else
    {
        capture_width = ds_video_graph_->Width();
        capture_height = ds_video_graph_->Height();
        x264_encoder_->Initialize(capture_width, capture_height, webcam_bitrate_, 25);
    }

    // 开始循环获取每一帧，并编码
    unsigned long yuvimg_size = capture_width * capture_height * 3 / 2;
    unsigned char* yuvbuf = (unsigned char*)malloc(yuvimg_size);
    unsigned char* x264buf = (unsigned char*)malloc(yuvimg_size*100);
    int x264buf_len = 0;

    __int64 last_tick = 0;
    unsigned int timestamp = 0;
    unsigned int last_idr_timestamp = 0;

    bool is_first = true;
	int m_iH264 = 0;
    char* tmpbuf = (char*)malloc(capture_width * capture_height * 3);

    bool is_have_capture = ds_video_graph_->IsCreateOK();
    if (is_capture_webcam_)
    {
        is_capture_webcam_ = is_have_capture;
    }

    while (false == IsStop())
    {
        unsigned int now_tick = ::GetTickCount();
        unsigned int next_tick = now_tick + 100;

        if (is_capture_webcam_)
        {
            char* rgbbuf = ds_video_graph_->GetBuffer();
            if (rgbbuf)
            {
                if (last_tick == 0)
                {
                    last_tick = ::GetTickCount();
                }
                else
                {
                    __int64 nowtick = ::GetTickCount();
                    if (nowtick < last_tick)
                        timestamp = 0;
                    else
                        timestamp += (nowtick - last_tick);
                    last_tick = nowtick;
                }

                bool is_keyframe = false;
                if (timestamp - last_idr_timestamp >= 2000 || timestamp == 0)
                {
                    is_keyframe = true;
                    last_idr_timestamp = timestamp;
                }

                if (is_save_picture_)
                {
                    SaveRgb2Bmp(rgbbuf, ds_video_graph_->Width(),  ds_video_graph_->Height());
                    is_save_picture_ = false;
                }

                if (is_capture_screen_)
                {
                    GetDesktopImage(rgbbuf, capx1_, capy1_, ds_video_graph_->Width(),  ds_video_graph_->Height(), tmpbuf);
                    // 发往预览窗口
                    unsigned int* buflen = new unsigned int;
                    char* frame_buf = new char[screen_width_ * screen_height_ * 3];
                    //memcpy(frame_buf, tmpbuf, screen_width_ * screen_height_ * 3);
                   /* int cappos = 0;
                    for (int yi = 0; yi < screen_height_; yi++)
                    {
                        char* pbuf = frame_buf + screen_width_ * (screen_height_ - 1 - yi) * 3 ; 
                        for (int xi = 0; xi < screen_width_; xi++)
                        {

                            *(pbuf++) = tmpbuf[cappos++];
                            *(pbuf++) = tmpbuf[cappos++];
                            *(pbuf++) = tmpbuf[cappos++];
                        }
                    }*/
                    *buflen = screen_width_ * screen_height_ * 3;
                    PostMessage(gFrameWnd, WM_FRAME, (WPARAM)buflen, (LPARAM)frame_buf);
                    RGB2YUV420((LPBYTE)tmpbuf, screen_width_, screen_height_, (LPBYTE)yuvbuf,  &yuvimg_size);
                }
                else
                {
                    // 发往预览窗口
                    unsigned int* buflen = new unsigned int;
                    unsigned char* frame_buf = new unsigned char[capture_width * capture_height * 3];
                    memcpy(frame_buf, rgbbuf, capture_width * capture_height * 3);
                    *buflen = capture_width * capture_height * 3;
                    PostMessage(gFrameWnd, WM_FRAME, (WPARAM)buflen, (LPARAM)frame_buf);

                    RGB2YUV420((LPBYTE)rgbbuf, capture_width, capture_height, (LPBYTE)yuvbuf,  &yuvimg_size);
                }

                x264buf_len = yuvimg_size*100;
				//x264buf_len = yuvimg_size;
				//if(m_iH264<1)
				//	is_keyframe=TRUE;
				//else if(m_iH264>24)
				//{
				//	is_keyframe=TRUE;
				//	m_iH264=0;
				//}
				//else
				//	is_keyframe=FALSE;
                x264_encoder_->Encode(yuvbuf, x264buf, x264buf_len, is_keyframe);
				m_iH264++;
                if (is_first)
                {
                    listener_->OnSPSAndPPS(x264_encoder_->SPS(), x264_encoder_->SPSSize(),
                        x264_encoder_->PPS(), x264_encoder_->PPSSize());
                    is_first = false;
                }

                if (x264buf_len > 0)
                {
                    if (fp_264)
                    {
                        fwrite(x264buf, x264buf_len, 1, fp_264);
                    }

                    if (listener_ && x264buf_len)
                    {
                        base::DataBuffer* dataBuf = new base::DataBuffer((char*)x264buf, x264buf_len);
                        listener_->OnCaptureVideoBuffer(dataBuf, timestamp, is_keyframe);
                    }
                }
            }
            ds_video_graph_->ReleaseBuffer(rgbbuf);
        }
        else
        {
            if (last_tick == 0)
            {
                last_tick = ::GetTickCount();
            }
            else
            {
                __int64 nowtick = ::GetTickCount();
                if (nowtick < last_tick)
                    timestamp = 0;
                else
                    timestamp += (nowtick - last_tick);
                last_tick = nowtick;
            }

            bool is_keyframe = false;
           // if (timestamp - last_idr_timestamp >= 3000 || timestamp == 0)
		    if (timestamp - last_idr_timestamp >= 2000 || timestamp == 0)
            {
                is_keyframe = true;
                last_idr_timestamp = timestamp;
            }

            GetDesktopImage(0, 0, 0, 0,  0, tmpbuf);
            // 发往预览窗口
            unsigned int* buflen = new unsigned int;
            char* frame_buf = new char[screen_width_ * screen_height_ * 3];
            memcpy(frame_buf, tmpbuf, screen_width_ * screen_height_ * 3);
          /* int cappos = 0;
            for (int yi = 0; yi < screen_height_; yi++)
            {
                char* pbuf = frame_buf + screen_width_ * (screen_height_ - 1 - yi) * 3 ; 
                for (int xi = 0; xi < screen_width_; xi++)
                {

                    *(pbuf++) = tmpbuf[cappos++];
                    *(pbuf++) = tmpbuf[cappos++];
                    *(pbuf++) = tmpbuf[cappos++];
                }
            }*/
            *buflen = screen_width_ * screen_height_ * 3;
            PostMessage(gFrameWnd, WM_FRAME, (WPARAM)buflen, (LPARAM)frame_buf);

            RGB2YUV420((LPBYTE)tmpbuf, screen_width_, screen_height_, (LPBYTE)yuvbuf,  &yuvimg_size);

			///fwrite(yuvbuf,yuvimg_size, 1, fp_yuv);
			x264buf_len = yuvimg_size*100;
			//x264buf_len = yuvimg_size;
			/*if(m_iH264<1)
				is_keyframe=TRUE;
			else if(m_iH264>24)
			{
				is_keyframe=TRUE;
				m_iH264=0;
			}
			else
				is_keyframe=FALSE;*/
			x264_encoder_->Encode(yuvbuf, x264buf, x264buf_len, is_keyframe);
		//	m_iH264++;
            if (is_first)
            {
                listener_->OnSPSAndPPS(x264_encoder_->SPS(), x264_encoder_->SPSSize(),
                    x264_encoder_->PPS(), x264_encoder_->PPSSize());
                is_first = false;
            }


            if (x264buf_len > 0)
            {
                if (fp_264)
                {
                    fwrite(x264buf, x264buf_len, 1, fp_264);
                }

                if (listener_ && x264buf_len)
                {
                    base::DataBuffer* dataBuf = new base::DataBuffer((char*)x264buf, x264buf_len);
                    listener_->OnCaptureVideoBuffer(dataBuf, timestamp, is_keyframe);
                }
            }
        }

        //Sleep(1000/ds_video_graph_->FPS());
        now_tick = ::GetTickCount();
        if (next_tick > now_tick)
        {
            Sleep(next_tick-now_tick);
        }
    }

    free(tmpbuf);
    free(yuvbuf);
    free(x264buf);

    if (fp_264)
    {
        fclose(fp_264);
    }
}

void  VideoEncoderThread::SetOutputFilename(const std::string& filename)
{
    filename_264_ = filename;
}

bool VideoEncoderThread::RGB2YUV420(LPBYTE RgbBuf,UINT nWidth,UINT nHeight,LPBYTE yuvBuf,unsigned long *len)
{
    int i, j; 
    unsigned char*bufY, *bufU, *bufV, *bufRGB,*bufYuv; 
    //memset(yuvBuf,0,(unsigned int )*len);
    bufY = yuvBuf; 
    bufV = yuvBuf + nWidth * nHeight; 
    bufU = bufV + (nWidth * nHeight* 1/4); 
    *len = 0; 
    unsigned char y, u, v, r, g, b,testu,testv; 
    unsigned int ylen = nWidth * nHeight;
    unsigned int ulen = (nWidth * nHeight)/4;
    unsigned int vlen = (nWidth * nHeight)/4; 
    //for (j = 0; j<nHeight;j++)
    for (j = nHeight-1; j >= 0;j--)
    {
        bufRGB = RgbBuf + nWidth * (nHeight - 1 - j) * 3 ; 
        for (i = nWidth-1; i >= 0;i--)
        {
            int pos = nWidth * i + j;
            r = *(bufRGB++);
            g = *(bufRGB++);
            b = *(bufRGB++);

            y = (unsigned char)( ( 66 * r + 129 * g + 25 * b + 128) >> 8) + 16 ;           
            u = (unsigned char)( ( -38 * r - 74 * g + 112 * b + 128) >> 8) + 128 ;           
            v = (unsigned char)( ( 112 * r - 94 * g - 18 * b + 128) >> 8) + 128 ;
            *(bufY++) = max( 0, min(y, 255 ));

            if (j%2==0&&i%2 ==0)
            {
                if (u>255)
                {
                    u=255;
                }
                if (u<0)
                {
                    u = 0;
                }
                *(bufU++) =u;
                //存u分量
            }
            else
            {
                //存v分量
                if (i%2==0)
                {
                    if (v>255)
                    {
                        v = 255;
                    }
                    if (v<0)
                    {
                        v = 0;
                    }
                    *(bufV++) =v;

                }
            }

        }

    }
    *len = nWidth * nHeight+(nWidth * nHeight)/2;
    return true;
}

void VideoEncoderThread::SaveRgb2Bmp(char* rgbbuf, unsigned int width, unsigned int height)
{
    BITMAPINFO bitmapinfo;
    ZeroMemory(&bitmapinfo,sizeof(BITMAPINFO));
    bitmapinfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapinfo.bmiHeader.biWidth = width;
    bitmapinfo.bmiHeader.biHeight = -height;
    bitmapinfo.bmiHeader.biPlanes = 1;
    bitmapinfo.bmiHeader.biBitCount =24;
    bitmapinfo.bmiHeader.biXPelsPerMeter = 0;
    bitmapinfo.bmiHeader.biYPelsPerMeter = 0;
    bitmapinfo.bmiHeader.biSizeImage = width*height;
    bitmapinfo.bmiHeader.biClrUsed = 0;        
    bitmapinfo.bmiHeader.biClrImportant = 0;

    BITMAPFILEHEADER bmpHeader;
    ZeroMemory(&bmpHeader,sizeof(BITMAPFILEHEADER));
    bmpHeader.bfType = 0x4D42;
    bmpHeader.bfOffBits = sizeof(BITMAPINFOHEADER)+sizeof(BITMAPFILEHEADER);
    bmpHeader.bfSize = bmpHeader.bfOffBits + width*height*3;

    FILE* fp = fopen("./CameraCodingCapture.bmp", "wb");
    if (fp)
    {
        fwrite(&bmpHeader, 1, sizeof(BITMAPFILEHEADER), fp);
        fwrite(&(bitmapinfo.bmiHeader), 1, sizeof(BITMAPINFOHEADER), fp);
        fwrite(rgbbuf, 1, width*height*3, fp);
        fclose(fp);
    }
}

void VideoEncoderThread::SetCaptureWebcam(bool isCapture)
{
    is_capture_webcam_ = isCapture;
    if (is_capture_webcam_)
    {
        is_capture_webcam_ = ds_video_graph_->IsCreateOK();
    }
}

void VideoEncoderThread::SetCaptureScreen(bool isCapture)
{
    is_capture_screen_ = isCapture;
}

#include "stdafx.h"
#include "VideoEncoderThread.h"

#include "dshow/DSCapture.h"
#include "dshow/DSVideoGraph.h"
#include "X264Encoder.h"
#include "LiveImage.h"

VideoEncoderThread::VideoEncoderThread(DSVideoGraph* dsVideoGraph, 
    int webcamBitrate, int screenBitrate)
    : ds_video_graph_(dsVideoGraph), x264_encoder_(new X264Encoder)
    , is_save_picture_(false)
{
    encoder_width_ = 0;
    encoder_height_ = 0;
    webcam_bitrate_ = webcamBitrate;
    screen_bitrate_ = screenBitrate;
    webcam_x_ = 0;
    webcam_y_ = 0;

    HDC hscreen = ::GetDC(NULL);
    screen_width_ = ::GetDeviceCaps(hscreen, HORZRES);
    screen_height_ = ::GetDeviceCaps(hscreen, VERTRES);
    cropx_ = 0;
    cropy_ = 0;
    crop_width_ = screen_width_;
    crop_height_ = screen_height_;
    int bits_per_pixel = ::GetDeviceCaps(hscreen, BITSPIXEL);
    switch (bits_per_pixel)
    {
    case 8:
        screen_chroma_ = 8;  screen_pix_bytes_ = 1; break;
    case 15:
    case 16:    /* Yes it is really 15 bits (when using BI_RGB) */
        screen_chroma_ = 16; screen_pix_bytes_ = 2; break;
    case 24:
        screen_chroma_ = 24; screen_pix_bytes_ = 3; break;
    case 32: 
        screen_chroma_ = 32; screen_pix_bytes_ = 4; break;
    default:
        screen_chroma_ = 24; screen_pix_bytes_ = 3;
    }
    screen_buf_size_ = screen_width_ * screen_height_ * screen_pix_bytes_;

    listener_ = NULL;
    is_capture_webcam_ = true;
    is_capture_screen_ = true;

    live_image_ = NULL;
}

VideoEncoderThread::~VideoEncoderThread()
{
    delete x264_encoder_;
    delete live_image_;
}

void PaintMousePointer(HDC destHdc, int cropX, int cropY, int cropWidth, int cropHeight);

void VideoEncoderThread::GetDesktopImage(char* outRgbbuf)
{
    HDC         hScrDC, hMemDC;
    HGDIOBJ     hOldBitmap , hBitmap;

    // create a DC for the screen and create     
    // a memory DC compatible to screen DC    
    //hScrDC = CreateDC(_T("DISPLAY"), NULL, NULL, NULL);     
    hScrDC = GetDC(NULL);
    hMemDC = CreateCompatibleDC(hScrDC);      // get points of rectangle to grab 

    // create a bitmap compatible with the screen DC     
    hBitmap = CreateCompatibleBitmap(hScrDC, crop_width_, crop_height_);      

    // select new bitmap into memory DC     
    hOldBitmap =   SelectObject (hMemDC, hBitmap);

    // bitblt screen DC to memory DC     
    BitBlt(hMemDC, 0, 0, crop_width_, crop_height_, hScrDC, cropx_, cropy_, SRCCOPY);
    //BitBlt(hMemDC, capX, capY, capWidth, capHeight, );
    PaintMousePointer(hMemDC, cropx_, cropy_, crop_width_, crop_height_);

    // select old bitmap back into memory DC and get handle to     
    // bitmap of the screen          
    hBitmap = SelectObject(hMemDC, hOldBitmap);      


    BITMAPINFO bmp_info_;
    memset( &bmp_info_, 0, sizeof(bmp_info_) );
    bmp_info_.bmiHeader.biSize = sizeof( BITMAPINFOHEADER );
    bmp_info_.bmiHeader.biWidth = crop_width_;
    bmp_info_.bmiHeader.biHeight = crop_height_;
    bmp_info_.bmiHeader.biPlanes = 1;
    bmp_info_.bmiHeader.biBitCount = screen_chroma_;
    bmp_info_.bmiHeader.biCompression = BI_RGB;
    bmp_info_.bmiHeader.biSizeImage = crop_width_*crop_height_*screen_pix_bytes_;

    int ret = GetDIBits(hMemDC, (HBITMAP)hBitmap, 0, crop_height_, outRgbbuf, &bmp_info_, DIB_RGB_COLORS);

    // clean up      
    DeleteDC(hScrDC);     
    DeleteDC(hMemDC);
    DeleteObject(hBitmap);
    DeleteObject(hOldBitmap);
}

void PaintMousePointer(HDC destHdc, int cropX, int cropY, int cropWidth, int cropHeight)
{
    CURSORINFO ci = {0};

    ci.cbSize = sizeof(ci);

    if (GetCursorInfo(&ci))
    {
        HCURSOR icon = CopyCursor(ci.hCursor);
        ICONINFO info;
        POINT pos;
        info.hbmMask = NULL;
        info.hbmColor = NULL;

        if (ci.flags != CURSOR_SHOWING)
            return;

        if (!icon)
        {
            /* Use the standard arrow cursor as a fallback.
             * You'll probably only hit this in Wine, which can't fetch
             * the current system cursor. */
            icon = CopyCursor(LoadCursor(NULL, IDC_ARROW));
        }

        if (!GetIconInfo(icon, &info))
        {
            goto icon_error;
        }

        pos.x = ci.ptScreenPos.x - cropX - info.xHotspot;
        pos.y = ci.ptScreenPos.y - cropY - info.yHotspot;

        if (pos.x >= 0 && pos.x <= cropWidth &&
                pos.y >= 0 && pos.y <= cropHeight)
        {
            if (!DrawIcon(destHdc, pos.x, pos.y, icon))
            {
                //CURSOR_ERROR("Couldn't draw icon");
            }
        }

icon_error:
        if (info.hbmMask)
            DeleteObject(info.hbmMask);
        if (info.hbmColor)
            DeleteObject(info.hbmColor);
        if (icon)
            DestroyCursor(icon);
    }
}

void SaveRgb2Bmp(char* rgbbuf, unsigned int width, unsigned int height, int pixCount)
{
    int pixbytes = (pixCount+1)/8;
    BITMAPINFO bitmapinfo;
    ZeroMemory(&bitmapinfo,sizeof(BITMAPINFO));
    bitmapinfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapinfo.bmiHeader.biWidth = width;
    bitmapinfo.bmiHeader.biHeight = -height;
    bitmapinfo.bmiHeader.biPlanes = 1;
    bitmapinfo.bmiHeader.biBitCount =pixCount;
    bitmapinfo.bmiHeader.biXPelsPerMeter = 0;
    bitmapinfo.bmiHeader.biYPelsPerMeter = 0;
    bitmapinfo.bmiHeader.biSizeImage = width*height*pixbytes;
    bitmapinfo.bmiHeader.biClrUsed = 0;        
    bitmapinfo.bmiHeader.biClrImportant = 0;

    BITMAPFILEHEADER bmpHeader;
    ZeroMemory(&bmpHeader,sizeof(BITMAPFILEHEADER));
    bmpHeader.bfType = 0x4D42;
    bmpHeader.bfOffBits = sizeof(BITMAPINFOHEADER)+sizeof(BITMAPFILEHEADER);
    bmpHeader.bfSize = bmpHeader.bfOffBits + width*height*pixbytes;

    FILE* fp = fopen("./CameraCodingCapture.bmp", "wb");
    if (fp)
    {
        fwrite(&bmpHeader, 1, sizeof(BITMAPFILEHEADER), fp);
        fwrite(&(bitmapinfo.bmiHeader), 1, sizeof(BITMAPINFOHEADER), fp);
        fwrite(rgbbuf, 1, width*height*pixbytes, fp);
        fclose(fp);
    }
}

void RGB32ToYUV420(char* RgbBuf,int nWidth, int nHeight, char* yuvBuf)
{
    char*bufY, *bufU, *bufV; 
    bufY = yuvBuf; 
    bufU = yuvBuf + nWidth * nHeight; 
    bufV = bufU + (nWidth * nHeight* 1/4); 

    unsigned char y, u, v, r, g, b,testu,testv; 
    for (int j = 0; j<nHeight;j++)
    {
        char* bufRGB = RgbBuf + nWidth * (nHeight - 1 - j) * 4 ; 
        for (int i = 0;i<nWidth;i++)
        {
            int pos = nWidth * i + j;
            b = *(bufRGB++);
            g = *(bufRGB++);
            r = *(bufRGB++);
            bufRGB++;   // a

            y = (unsigned char)( ( 66 * r + 129 * g + 25 * b + 128) >> 8) + 16 ;           
            u = (unsigned char)( ( -38 * r - 74 * g + 112 * b + 128) >> 8) + 128 ;           
            v = (unsigned char)( ( 112 * r - 94 * g - 18 * b + 128) >> 8) + 128 ;
            *(bufY++) = max( 0, min(y, 255 ));

            if (j%2==0&&i%2 ==0)
            {
                if (u>255) u=255;
                if (u<0) u = 0;
                *(bufU++) =u;
            }
            else
            {
                //存v分量
                if (i%2==0)
                {
                    if (v>255) v = 255;
                    if (v<0) v = 0;
                    *(bufV++) =v;
                }
            }
        }
    }
}

void VideoEncoderThread::Run()
{
    int origin_width = 0;
    int origin_height = 0;
    if (is_capture_screen_)
    {
        origin_width = crop_width_;
        origin_height = crop_height_;

        if (encoder_width_ == 0)
            encoder_width_ = crop_width_;
        if (encoder_height_ == 0)
            encoder_height_ = crop_height_;

        if (encoder_height_ % 2 != 0) encoder_height_++;
        x264_encoder_->Initialize(encoder_width_, encoder_height_, screen_bitrate_, 10);
    }
    else
    {
        origin_width = ds_video_graph_->Width();
        origin_height = ds_video_graph_->Height();

        //if (encoder_width_ == 0)
            encoder_width_ = ds_video_graph_->Width();
        //if (encoder_height_ == 0)
            encoder_height_ = ds_video_graph_->Height();

        if (encoder_height_ % 2 != 0) encoder_height_++;
        x264_encoder_->Initialize(encoder_width_, encoder_height_, webcam_bitrate_, 10);
    }

    live_image_ = new LiveImage;
    live_image_->AllocImage(encoder_width_, encoder_height_, 32);

    unsigned long yuvimg_size = encoder_width_ * encoder_height_ * 3 / 2;
    unsigned char* yuvbuf = (unsigned char*)malloc(yuvimg_size);

    int x264buf_len = 0;

    __int64 last_tick = 0;
    unsigned int timestamp = 0;
    unsigned int last_idr_timestamp = 0;

    bool is_first = true;
    char* tmpbuf = (char*)malloc(origin_width * origin_height * 4);
    //char* ttbuf = (char*)_aligned_malloc(encoder_width_ * encoder_height_ * 3, 4);

    bool is_have_capture = ds_video_graph_->IsCreateOK();
    if (is_capture_webcam_)
    {
        is_capture_webcam_ = is_have_capture;
    }

    char* tmp_rgb_line = new char[ds_video_graph_->Width()*4];

    double interval = 1000.0 / 10;
    while (false == IsStop())
    {
        unsigned int now_tick = ::GetTickCount();
        unsigned int next_tick = now_tick + interval;

        if (is_capture_webcam_)
        {
            char* rgbbuf = ds_video_graph_->GetBuffer();
            if (rgbbuf)
            {
                int cam_width = ds_video_graph_->Width();
                int cam_height = ds_video_graph_->Height();

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
                        timestamp += static_cast<unsigned int>(nowtick - last_tick);
                    last_tick = nowtick;
                }

                bool is_keyframe = false;
                if (timestamp - last_idr_timestamp >= 5000 || timestamp == 0)
                {
                    is_keyframe = true;
                    last_idr_timestamp = timestamp;
                }

                if (is_save_picture_)
                {
                    //SaveRgb2Bmp(rgbbuf, ds_video_graph_->Width(),  ds_video_graph_->Height());
                    is_save_picture_ = false;
                }

                if (is_capture_screen_)
                {
                    GetDesktopImage(tmpbuf);
                    live_image_->DrawImage(tmpbuf, screen_chroma_, crop_width_, crop_height_,
                        0, 0, encoder_width_, encoder_height_);
                    live_image_->DrawImage(rgbbuf, 32, ds_video_graph_->Width(),  ds_video_graph_->Height(),
                        webcam_x_, webcam_y_, ds_video_graph_->Width(),  ds_video_graph_->Height());
                    live_image_->FetchBuffer(tmpbuf, 32);

                    // 发往预览窗口
                    //if (is_preview_)
                    {
                        unsigned int* buflen = new unsigned int;
                        char* frame_buf = new char[encoder_width_ * encoder_height_ * 4];
                        live_image_->FetchBuffer(frame_buf, 32);
                        //                     int cappos = 0;
                        //                     for (int yi = 0; yi < encoder_height_; yi++)
                        //                     {
                        //                         char* pbuf = frame_buf + encoder_width_ * yi * 3 ; 
                        //                         for (int xi = 0; xi < encoder_width_; xi++)
                        //                         {
                        // 
                        //                             *(pbuf++) = tmpbuf[cappos++];
                        //                             *(pbuf++) = tmpbuf[cappos++];
                        //                             *(pbuf++) = tmpbuf[cappos++];
                        //                         }
                        //                     }
                        *buflen = encoder_width_ * encoder_height_ * 4;
                        PostMessage(gFrameWnd, WM_FRAME, (WPARAM)buflen, (LPARAM)frame_buf);
                    }

                    RGB32ToYUV420(tmpbuf, encoder_width_, encoder_height_, (char*)yuvbuf);
                }
                else
                {
//                     live_image_->DrawImage(rgbbuf, 32, ds_video_graph_->Width(),  ds_video_graph_->Height(),
//                         webcam_x_, webcam_y_, ds_video_graph_->Width(),  ds_video_graph_->Height());
                    // 发往预览窗口
                    //if (is_preview_)
                    {
                        unsigned int* buflen = new unsigned int;
                        char* frame_buf = new char[encoder_width_ * encoder_height_ * 4];
                        //live_image_->FetchBuffer(frame_buf, 32);
                        *buflen = encoder_width_ * encoder_height_ * 4;
                        memcpy(frame_buf, rgbbuf, *buflen);
                        PostMessage(gFrameWnd, WM_FRAME, (WPARAM)buflen, (LPARAM)frame_buf);
                    }

                    RGB32ToYUV420(rgbbuf, encoder_width_, encoder_height_, (char*)yuvbuf);
                }

                x264buf_len = yuvimg_size*100;
                char* x264buf = new char[encoder_width_*encoder_height_*4];
                x264_encoder_->Encode(yuvbuf, (unsigned char*)x264buf, x264buf_len, is_keyframe);
                if (is_first)
                {
                    listener_->OnSPSAndPPS(x264_encoder_->SPS(), x264_encoder_->SPSSize(),
                        x264_encoder_->PPS(), x264_encoder_->PPSSize());
                    is_first = false;
                }

                if (x264buf_len > 0)
                {
                    if (listener_ && x264buf_len > 0)
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
                    timestamp += static_cast<unsigned int>(nowtick - last_tick);
                last_tick = nowtick;
            }

            bool is_keyframe = false;
            if (timestamp - last_idr_timestamp >= 5000 || timestamp == 0)
            {
                is_keyframe = true;
                last_idr_timestamp = timestamp;
            }

            GetDesktopImage(tmpbuf);
            SaveRgb2Bmp(tmpbuf, crop_width_, crop_height_, 32);
            if (screen_chroma_ != 32)
            {
                live_image_->DrawImage(tmpbuf, screen_chroma_, crop_width_, crop_height_,
                    0, 0, encoder_width_, encoder_height_);
                live_image_->FetchBuffer(tmpbuf, 32);
            }

            // 发往预览窗口
            //if (is_preview_)
            {
                unsigned int* buflen = new unsigned int;
                char* frame_buf = new char[encoder_width_ * encoder_height_ * 4];
                //live_image_->FetchBuffer(frame_buf, 32);
                *buflen = encoder_width_ * encoder_height_ * 4;
                memcpy(frame_buf, tmpbuf, *buflen);
                PostMessage(gFrameWnd, WM_FRAME, (WPARAM)buflen, (LPARAM)frame_buf);
            }

            RGB32ToYUV420(tmpbuf, encoder_width_, encoder_height_, (char*)yuvbuf);

            x264buf_len = yuvimg_size*100;
            char* x264buf = new char[encoder_width_*encoder_height_*4];
            x264_encoder_->Encode(yuvbuf, (unsigned char*)x264buf, x264buf_len, is_keyframe);
            if (is_first)
            {
                listener_->OnSPSAndPPS(x264_encoder_->SPS(), x264_encoder_->SPSSize(),
                    x264_encoder_->PPS(), x264_encoder_->PPSSize());
                is_first = false;
            }

            if (x264buf_len > 0)
            {
                if (listener_ && x264buf_len > 0)
                {
                    base::DataBuffer* dataBuf = new base::DataBuffer((char*)x264buf, x264buf_len);
                    listener_->OnCaptureVideoBuffer(dataBuf, timestamp, is_keyframe);
                }
            }
            else
            {
                delete[] x264buf;
            }
			delete[] x264buf;
        }

        //Sleep(1000/ds_video_graph_->FPS());
        now_tick = ::GetTickCount();
        if (next_tick > now_tick)
        {
            Sleep(next_tick-now_tick);
        }
    }
	
    delete[] tmp_rgb_line;

    //_aligned_free(ttbuf);
    free(tmpbuf);
    free(yuvbuf);
}

bool VideoEncoderThread::RGB2YUV420(LPBYTE RgbBuf,UINT nWidth,UINT nHeight,LPBYTE yuvBuf,unsigned long *len)
{
    unsigned int i, j; 
    unsigned char*bufY, *bufU, *bufV, *bufRGB; 
    //memset(yuvBuf,0,(unsigned int )*len);
    bufY = yuvBuf; 
    bufU = yuvBuf + nWidth * nHeight; 
    bufV = bufU + (nWidth * nHeight* 1/4); 
    *len = 0; 
    unsigned char y, u, v, r, g, b; 
    unsigned int ylen = nWidth * nHeight;
    unsigned int ulen = (nWidth * nHeight)/4;
    unsigned int vlen = (nWidth * nHeight)/4; 
    for (j = 0; j<nHeight;j++)
    {
        bufRGB = RgbBuf + nWidth * (nHeight - 1 - j) * 3 ; 
        for (i = 0;i<nWidth;i++)
        {
            int pos = nWidth * i + j;
            b = *(bufRGB++);
            g = *(bufRGB++);
            r = *(bufRGB++);

            y = (unsigned char)( ( 66 * r + 129 * g + 25 * b + 128) >> 8) + 16 ;           
            u = (unsigned char)( ( -38 * r - 74 * g + 112 * b + 128) >> 8) + 128 ;           
            v = (unsigned char)( ( 112 * r - 94 * g - 18 * b + 128) >> 8) + 128 ;
            *(bufY++) = max( 0, min(y, 255 ));

            if (j%2==0 && i%2 ==0)
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

bool VideoEncoderThread::RGB2YUV420_r(LPBYTE RgbBuf,UINT nWidth,UINT nHeight,LPBYTE yuvBuf,unsigned long *len)
{
    int i, j; 
    unsigned char*bufY, *bufU, *bufV, *bufRGB; 
    //memset(yuvBuf,0,(unsigned int )*len);
    bufY = yuvBuf; 
    bufV = yuvBuf + nWidth * nHeight; 
    bufU = bufV + (nWidth * nHeight* 1/4); 
    *len = 0; 
    unsigned char y, u, v, r, g, b; 
    unsigned int ylen = nWidth * nHeight;
    unsigned int ulen = (nWidth * nHeight)/4;
    unsigned int vlen = (nWidth * nHeight)/4; 
    for (j = nHeight-1; j >= 0;j--)
    {
        bufRGB = RgbBuf + nWidth * (nHeight - 1 - j) * 3 ; 
        for (i = nWidth-1; i >= 0;i--)
        {
            //unsigned char* prgb = bufRGB + i*3;
            //int pos = nWidth * i + j;
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

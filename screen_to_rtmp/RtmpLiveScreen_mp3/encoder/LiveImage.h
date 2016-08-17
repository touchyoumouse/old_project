#ifndef _LIVE_IMAGE_H_
#define _LIVE_IMAGE_H_

#include "libfreeimage/FreeImage.h"

class LiveImage
{
public:
    LiveImage();

    ~LiveImage();

    void AllocImage(int width, int height, int pixbits);

    void AlignImage(char* imgBuf, int pixbits, int width, int height);

    void DrawImage(const char* srcBuf, int pixbits,
        int srcWidth, int srcHeight,
        int dstX, int dstY, int dstWidth, int dstHeight);

    void FillBackground();

    void FetchBuffer(char* outBuf, int pixbits);

    bool IsInit() { return width_ != 0; }

private:
    FIBITMAP* image_;

    int width_;
    int height_;
};

#endif // _LIVE_IMAGE_H_

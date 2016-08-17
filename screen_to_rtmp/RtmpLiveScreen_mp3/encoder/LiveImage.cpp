#include "stdafx.h"
#include "LiveImage.h"

#pragma comment(lib, "libfreeimage/FreeImage.lib")

LiveImage::LiveImage()
{
    width_ = 0;
    height_ = 0;

    image_ = 0;
}

LiveImage::~LiveImage()
{
    if (image_)
    {
        FreeImage_Unload(image_);
    }
}

void LiveImage::AllocImage(int width, int height, int pixbits)
{
    width_ = width;
    height_ = height;

    image_ = FreeImage_Allocate(width, height, pixbits);
    RGBQUAD color;
    color.rgbBlue = 0;
    color.rgbGreen = 0;
    color.rgbRed = 0;
    color.rgbReserved = 0;
    FreeImage_FillBackground(image_, &color);
}

void LiveImage::AlignImage(char* imgBuf, int pixbits, int width, int height)
{
    if (image_)
    {
        FreeImage_Unload(image_);
        image_ = 0;
    }

    width_ = width;
    height_ = height;

    int bpp = pixbits;
    int pix_byte = bpp / 8;
    int pitch = width * height * pix_byte/height;

    image_ = FreeImage_ConvertFromRawBits((BYTE*)imgBuf, width, height, pitch, bpp, 0, 0, 0);
}

void LiveImage::DrawImage(const char* srcBuf, int pixbits,
    int srcWidth, int srcHeight,
    int dstX, int dstY, int dstWidth, int dstHeight)
{
    int bpp = pixbits;
    int pix_byte = bpp / 8;
    int src_pitch = srcWidth * srcHeight * pix_byte/srcHeight;
    FIBITMAP *src_img = FreeImage_ConvertFromRawBits((BYTE*)srcBuf, 
        srcWidth, srcHeight, src_pitch, bpp, 0, 0, 0);

    if (srcWidth != dstWidth || srcHeight != dstHeight)
    {
        FIBITMAP* tmp_img = FreeImage_Rescale(src_img, dstWidth, dstHeight, FILTER_BOX);
        FreeImage_Paste(image_, tmp_img, dstX, dstY, 255);
        FreeImage_Unload(tmp_img);
    }
    else
    {
        FreeImage_Paste(image_, src_img, dstX, dstY, 255);
    }
    FreeImage_Unload(src_img);
}

void LiveImage::FetchBuffer(char* outBuf, int pixbits)
{
    int pix_byte = pixbits / 8;

    int out_pitch = FreeImage_GetWidth(image_) * FreeImage_GetHeight(image_) *
        pix_byte / FreeImage_GetHeight(image_);
    FreeImage_ConvertToRawBits((BYTE*)outBuf, image_, out_pitch, pixbits, 0, 0, 0);
}

void LiveImage::FillBackground()
{
    RGBQUAD color;
    color.rgbBlue = 0;
    color.rgbGreen = 0;
    color.rgbRed = 0;
    color.rgbReserved = 0;
    FreeImage_FillBackground(image_, &color);
}

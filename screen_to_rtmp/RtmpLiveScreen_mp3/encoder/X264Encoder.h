#ifndef _X264_ENCODER_H_
#define _X264_ENCODER_H_

#include "stdint.h"
//#include <cstdio>

extern "C"
{
#include "libx264/x264.h"
}

struct TNAL
{
    int size;
    unsigned char* data;
    TNAL(): size(0), data(NULL) {}
};

class X264Encoder
{
public:
    X264Encoder();

    ~X264Encoder();

    // ��ʼ��������
    int Initialize(int iWidth, int iHeight, int iRateBit = 96, int iFps = 25);

    // ��һ֡������б��룬����NAL����
    int Encode(unsigned char* szYUVFrame, unsigned char* outBuf, int& outLen, bool& isKeyframe);

    // ����NAL����
    void CleanNAL(TNAL* pNALArray, int iNalNum);

    // ���ٱ�����
    int Destroy();

    char* SPS() { return sps_; }
    int SPSSize() { return sps_size_; }

    char* PPS() { return pps_; }
    int PPSSize() { return pps_size_; }

private:
    void FindSpsAndPPsFromBuf(const char* dataBuf, int bufLen);

private:
    x264_param_t m_param;
    x264_picture_t m_pic;
    x264_t* m_h;

    char* sps_;        // sequence parameter set
    int sps_size_;
    char* pps_;        // picture parameter set
    int pps_size_;
};

#endif // _X264_ENCODER_H_

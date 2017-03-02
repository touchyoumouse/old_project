#ifndef _FAAC_DECODER_H_
#define _FAAC_DECODER_H_

#include "../base/base.h"

#if defined(WIN32)
#include "../libfaad/faad.h"
#else
#include "faad.h"
#endif

class FAACDecoder
{
public:
    static void TryDecodeAudioCodecInfo(const char* aacBuf, int aacSize,
        char* outBuf, int* outSize);
public:
    FAACDecoder();

    ~FAACDecoder();

    /***
     * ��ʼ��AAC������
     * @param samrate: ������
     * @param channel: ����
     * @param bitsPerSample: λ����һ��Ϊ16
     * @returns:
     */
    void Init(unsigned int samRate, unsigned int channels, int bitsPerSample);

    /***
     * ����AAC����
     * @param inputBuf: �������ݣ�AAC��������
     * @param bufSize: �������ݳ���
     * @param outBuf: ����ڴ��ַ
     * @param outBufSize: ����ڴ��С
     */
    void Decode(unsigned char* inputBuf, unsigned int bufSize, 
        unsigned char* outBuf, unsigned int& outBufSize);

private:
    faacDecHandle faac_handle_;
    unsigned int sample_size_;
    bool is_first_;
};

#endif // _FAAC_DECODER_H_

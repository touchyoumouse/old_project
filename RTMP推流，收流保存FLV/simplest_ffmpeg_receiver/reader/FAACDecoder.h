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
     * 初始化AAC解码器
     * @param samrate: 采样率
     * @param channel: 声道
     * @param bitsPerSample: 位数，一般为16
     * @returns:
     */
    void Init(unsigned int samRate, unsigned int channels, int bitsPerSample);

    /***
     * 解码AAC数据
     * @param inputBuf: 输入数据，AAC编码数据
     * @param bufSize: 输入数据长度
     * @param outBuf: 输出内存地址
     * @param outBufSize: 输出内存大小
     */
    void Decode(unsigned char* inputBuf, unsigned int bufSize, 
        unsigned char* outBuf, unsigned int& outBufSize);

private:
    faacDecHandle faac_handle_;
    unsigned int sample_size_;
    bool is_first_;
};

#endif // _FAAC_DECODER_H_

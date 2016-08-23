/*******************************************************************************
 * faac_encoder.h
 * Copyright: (c) 2014 Haibin Du(haibindev.cnblogs.com). All rights reserved.
 * -----------------------------------------------------------------------------
 *
 * aac音频编码器，调用libfaac
 *
 * -----------------------------------------------------------------------------
 * 2015-6-4 19:44 - Created (Haibin Du)
 ******************************************************************************/

#ifndef _HDEV_FAAC_ENCODER_H_
#define _HDEV_FAAC_ENCODER_H_

#if defined(WIN32)
#include "libfaac/faac.h"
#else
#include "faac.h"
#endif

class FAACEncoder
{
public:
    FAACEncoder();

    ~FAACEncoder();

    /***
     * 初始化编码器
     * @param isOutAdts: 输出AAC数据是否需要带ADTS头
     * @param samRate: 采样率
     * @param channels: 声道数
     * @param bitsPerSample: 位数，一般为16
     * @param bitrate: 码率
     */
    void Init(bool isOutAdts, int samRate, int channels,
        int bitsPerSample, int bitrate);

    /***
     * 编码音频数据
     * @param inputBuf: 输入PCM音频数据
     * @param samCount: PCM Sample个数
     * @param outBuf: 输出AAC数据地址
     * @param bufSize: 输出数据大小
     * @returns:
     */
    void Encode(unsigned char* inputBuf, int samCount,
        unsigned char* outBuf, int& bufSize);

    void GetDecodeInfoBuf(unsigned char** ppbuf, int* psize);

    // 用 unsigned long，是因为faac的api中他们要被调用，类型为unsigned long
    unsigned long InputSamples() { return input_sams_; }

    unsigned long MaxOutBytes() { return max_output_bytes_; }

private:
    faacEncHandle faac_handle_;
    unsigned long input_sams_;
    unsigned long int max_output_bytes_;
};

#endif // _HDEV_FAAC_ENCODER_H_

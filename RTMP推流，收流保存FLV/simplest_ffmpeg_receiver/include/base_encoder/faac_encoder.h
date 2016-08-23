/*******************************************************************************
 * faac_encoder.h
 * Copyright: (c) 2014 Haibin Du(haibindev.cnblogs.com). All rights reserved.
 * -----------------------------------------------------------------------------
 *
 * aac��Ƶ������������libfaac
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
     * ��ʼ��������
     * @param isOutAdts: ���AAC�����Ƿ���Ҫ��ADTSͷ
     * @param samRate: ������
     * @param channels: ������
     * @param bitsPerSample: λ����һ��Ϊ16
     * @param bitrate: ����
     */
    void Init(bool isOutAdts, int samRate, int channels,
        int bitsPerSample, int bitrate);

    /***
     * ������Ƶ����
     * @param inputBuf: ����PCM��Ƶ����
     * @param samCount: PCM Sample����
     * @param outBuf: ���AAC���ݵ�ַ
     * @param bufSize: ������ݴ�С
     * @returns:
     */
    void Encode(unsigned char* inputBuf, int samCount,
        unsigned char* outBuf, int& bufSize);

    void GetDecodeInfoBuf(unsigned char** ppbuf, int* psize);

    // �� unsigned long������Ϊfaac��api������Ҫ�����ã�����Ϊunsigned long
    unsigned long InputSamples() { return input_sams_; }

    unsigned long MaxOutBytes() { return max_output_bytes_; }

private:
    faacEncHandle faac_handle_;
    unsigned long input_sams_;
    unsigned long int max_output_bytes_;
};

#endif // _HDEV_FAAC_ENCODER_H_

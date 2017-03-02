/*******************************************************************************
 * faac_encoder.cpp
 * Copyright: (c) 2014 Haibin Du(haibindev.cnblogs.com). All rights reserved.
 * -----------------------------------------------------------------------------
 *
 * -----------------------------------------------------------------------------
 * 2015-6-4 19:45 - Created (Haibin Du)
 ******************************************************************************/

#include "faac_encoder.h"

#include "../base/platform.h"

#if defined(HB_PLATFORM_WIN32)
#if defined(_DEBUG)
#pragma comment(lib, "./base_encoder/libfaac/libfaacd.lib")
#else
#pragma comment(lib, "./base_encoder/libfaac/libfaac.lib")
#endif
#endif

FAACEncoder::FAACEncoder()
{
    input_sams_ = 0;
    max_output_bytes_ = 0;
}

FAACEncoder::~FAACEncoder()
{
    faacEncClose(faac_handle_);
}

void FAACEncoder::Init(bool isOutAdts, int samRate, int channels,
    int bitsPerSample, int bitrate)
{
    faac_handle_ = faacEncOpen(samRate, channels, &input_sams_, &max_output_bytes_);
    faacEncConfigurationPtr enc_cfg = faacEncGetCurrentConfiguration(faac_handle_);
    switch(bitsPerSample)
    {
    case 16:
        enc_cfg->inputFormat=FAAC_INPUT_16BIT;
        break;
    case 24:
        enc_cfg->inputFormat=FAAC_INPUT_24BIT;
        break;
    case 32:
        enc_cfg->inputFormat=FAAC_INPUT_32BIT;
        break;
    default:
        enc_cfg->inputFormat=FAAC_INPUT_NULL;
        break;
    }
    enc_cfg->mpegVersion = MPEG4;
    enc_cfg->aacObjectType = LOW;
    enc_cfg->allowMidside = 1;
    enc_cfg->useLfe = 0;
    enc_cfg->useTns = 1;
    //enc_cfg->bitRate = bitrate/channels;
    enc_cfg->bitRate = 100;
    enc_cfg->bandWidth = 0;
    enc_cfg->quantqual = 100;
    enc_cfg->outputFormat = isOutAdts ? 1 : 0;
    faacEncSetConfiguration(faac_handle_, enc_cfg);
}

void FAACEncoder::Encode(unsigned char* inputBuf, int samCount,
    unsigned char* outBuf, int& bufSize)
{
    bufSize = faacEncEncode(faac_handle_, (int*)inputBuf, samCount,
        outBuf, max_output_bytes_);
}

void FAACEncoder::GetDecodeInfoBuf(unsigned char** ppbuf, int* psize)
{
    unsigned long infosize;
    faacEncGetDecoderSpecificInfo(faac_handle_, ppbuf, &infosize);
    if (psize)
    {
        *psize = infosize;
    }
}

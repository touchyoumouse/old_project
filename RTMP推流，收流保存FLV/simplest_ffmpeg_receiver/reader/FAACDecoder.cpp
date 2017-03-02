#include "FAACDecoder.h"

#include <cstring>

#include "../base/platform.h"

#if defined(HB_PLATFORM_WIN32)
#if defined(_DEBUG)
#pragma comment(lib, "libfaad/libfaadd.lib")
#else
#pragma comment(lib, "libfaad/libfaad.lib")
#endif
#endif

// static
void FAACDecoder::TryDecodeAudioCodecInfo(const char* aacBuf, int aacSize,
    char* outBuf, int* outSize)
{
    *outSize = 0;
    mp4AudioSpecificConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    NeAACDecAudioSpecificConfig((unsigned char*)aacBuf, aacSize, &cfg);
    if (cfg.channelsConfiguration > 0)
    {
        memcpy(outBuf, &cfg, sizeof(cfg));
        *outSize = sizeof(cfg);
    }
}

FAACDecoder::FAACDecoder() : faac_handle_(0), sample_size_(0)
{
    is_first_ = true;
}

FAACDecoder::~FAACDecoder()
{
    if (faac_handle_) faacDecClose(faac_handle_);
}

void FAACDecoder::Init(unsigned int samRate, unsigned int channels, int bitsPerSample)
{
    faac_handle_ = faacDecOpen();

    faacDecConfigurationPtr dec_cfg = faacDecGetCurrentConfiguration(faac_handle_);
    dec_cfg->defObjectType = LC;        /* Low Complexity (default) */
    dec_cfg->defSampleRate = samRate;
    switch(bitsPerSample)
    {
    case 16:
        dec_cfg->outputFormat=FAAD_FMT_16BIT;
        sample_size_ = 2;
        break;
    case 24:
        dec_cfg->outputFormat=FAAD_FMT_24BIT;
        sample_size_ = 3;
        break;
    case 32:
        dec_cfg->outputFormat=FAAD_FMT_32BIT;
        sample_size_ = 4;
        break;
    default:
        dec_cfg->outputFormat=FAAD_FMT_FLOAT;
        sample_size_ = 4;
        break;
    }

    char err = faacDecSetConfiguration(faac_handle_, dec_cfg);
}

void FAACDecoder::Decode(unsigned char* inputBuf, unsigned int bufSize, 
    unsigned char* outBuf, unsigned int& outBufSize)
{
     if (is_first_)
     {
         unsigned long sam_rate;
         unsigned char channel;
         unsigned char tmpval[2];
         tmpval[0] = 0x13;
         tmpval[1] = 0x88;
         char err = faacDecInit(faac_handle_, inputBuf, bufSize, &sam_rate, &channel);
         //char err = faacDecInit2(faac_handle_, tmpval, 2, &sam_rate, &channel);
         is_first_ = false;
     }
    faacDecFrameInfo frame_info;
    short * buf = (short *)faacDecDecode(faac_handle_, &frame_info, inputBuf, bufSize);
    if (buf)
    {
        outBufSize = frame_info.samples * sample_size_;
        for (int i = 0; i < frame_info.samples; i++)
        {
            outBuf[i*2] = (char)(buf[i] & 0xFF);
            outBuf[i*2+1] = (char)((buf[i] >> 8) & 0xFF);
        }
        //memcpy(outBuf, buf, outBufSize);
    }
    else
    {
        outBufSize = 0;
    }
}

#include "stdafx.h"
#include "MP3Encoder.h"

void Mp3LogFunction(const char *format, va_list ap)
{

}

MP3Encoder::MP3Encoder()
{
    gfp_ = lame_init();

    lame_set_errorf(gfp_, Mp3LogFunction);
    lame_set_debugf(gfp_, Mp3LogFunction);
    lame_set_msgf(gfp_, Mp3LogFunction);
}

MP3Encoder::~MP3Encoder()
{
    if (gfp_) Free();
}

void MP3Encoder::Init(unsigned int samRate, unsigned int channels)
{
    lame_set_num_channels(gfp_, channels);
    lame_set_in_samplerate(gfp_, samRate);
    lame_set_brate(gfp_, 64);           // 码率kbps
    lame_set_mode(gfp_, STEREO);  // 联合立体声
    lame_set_quality(gfp_, 5);          // 2=high  5 = medium  7=low

    lame_init_params(gfp_);
}

void MP3Encoder::Encode(unsigned char* inputBuf, unsigned int oneChannelSamCount, 
    unsigned char* outBuf, unsigned int& bufSize) 
{
    //bufSize = lame_encode_buffer_interleaved(gfp_, (short*)inputBuf, oneChannelSamCount, outBuf, bufSize);
    bufSize = lame_encode_buffer(gfp_, (short*)inputBuf, (short*)inputBuf,
        oneChannelSamCount, outBuf, bufSize);
}

int MP3Encoder::FrameSize()
{
    return lame_get_framesize(gfp_);
}

int MP3Encoder::MaxOutBytes(int oneChannelSamCount)
{
    return 1.25*oneChannelSamCount + 7200;
}

void MP3Encoder::Free()
{
    lame_close(gfp_);
    gfp_ = NULL;
}

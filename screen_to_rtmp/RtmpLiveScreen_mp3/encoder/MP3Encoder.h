#ifndef _MP3_ENCODER_H_
#define _MP3_ENCODER_H_

#include "libmp3lame/lame.h"

class MP3Encoder
{
public:
    MP3Encoder();

    ~MP3Encoder();

    void Init(unsigned int samRate, unsigned int channels);

    void Encode(unsigned char* inputBuf, unsigned int oneChannelSamCount, 
        unsigned char* outBuf, unsigned int& bufSize);

    int MaxOutBytes(int oneChannelSamCount);

    int FrameSize();

    void Free();

private:
    lame_global_flags* gfp_;
};

#endif // _MP3_ENCODER_H_

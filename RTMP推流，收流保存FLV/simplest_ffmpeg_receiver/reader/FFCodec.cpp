#include "FFCodec.h"

#include "../base/base.h"

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
}

void FFCodecInit()
{
    //avcodec_init();
    avcodec_register_all();
    avformat_network_init();
    av_register_all();
    
    av_log_set_level(AV_LOG_FATAL);
}

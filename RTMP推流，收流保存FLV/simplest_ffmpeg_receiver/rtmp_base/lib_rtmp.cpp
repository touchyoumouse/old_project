#include "lib_rtmp.h"

#include <cstdlib>
#include <cstring>

#include "base/platform.h"

#if defined(HB_PLATFORM_WIN32)
#if defined(_DEBUG)
#pragma comment(lib, "./rtmp_base/librtmp/librtmpd.lib")
#else
#pragma comment(lib, "./rtmp_base/librtmp/librtmp.lib")
#endif
#endif

#include "base/byte_stream.h"
#include "base/simple_logger.h"

LibRtmp::LibRtmp(bool isNeedLog)
{
    isNeedLog = false;

    if (isNeedLog)
    {
        flog_ = fopen("librtmp.log", "w");
        RTMP_LogSetLevel(RTMP_LOGALL);
        RTMP_LogSetOutput(flog_);
    }
    else
    {
        flog_ = NULL;
        RTMP_LogSetLevel(RTMP_LOGCRIT);
    }

    SIMPLE_LOG("try init\n");

    rtmp_ = RTMP_Alloc();   SIMPLE_LOG("\n");
    RTMP_Init(rtmp_);       SIMPLE_LOG("\n");
    RTMP_SetBufferMS(rtmp_, 300);

    streming_url_ = NULL;

    SIMPLE_LOG("init end\n");
}

LibRtmp::~LibRtmp()
{
    Close();
    RTMP_Free(rtmp_);

    if (streming_url_)
    {
        free(streming_url_);
        streming_url_ = NULL;
    }

    if (flog_) fclose(flog_);
}

bool LibRtmp::Open(const char* url)
{
    streming_url_ = (char*)calloc(strlen(url)+1, sizeof(char));
    strcpy(streming_url_, url);

    //AVal flashver = AVC("flashver");
    //AVal flashver_arg = AVC("WIN 9,0,115,0");
    AVal swfUrl = AVC("swfUrl");
    AVal swfUrl_arg = AVC("http://localhost/librtmp.swf");
    AVal pageUrl = AVC("pageUrl");
    AVal pageUrl_arg = AVC("http://localhost/librtmp.html");
    //RTMP_SetOpt(rtmp_, &flashver, &flashver_arg);
    RTMP_SetOpt(rtmp_, &swfUrl, &swfUrl_arg);
    RTMP_SetOpt(rtmp_, &pageUrl, &pageUrl_arg);
    //AVal record = AVC("record");
    //AVal record_arg = AVC("on");
    //RTMP_SetOpt(rtmp_, &record, &record_arg);

    int err = RTMP_SetupURL(rtmp_, streming_url_);
    if (err <= 0) return false;

    RTMP_EnableWrite(rtmp_);
    
    err = RTMP_Connect(rtmp_, NULL);
    if (err <= 0) return false;

    err = RTMP_ConnectStream(rtmp_, 0);
    if (err <= 0) return false;

    rtmp_->m_outChunkSize = 4*1024*1024;
    SendSetChunkSize(rtmp_->m_outChunkSize);

    return true;
}

void LibRtmp::Close()
{
    RTMP_Close(rtmp_);
}

bool LibRtmp::Send(const char* buf, int bufLen, int type, unsigned int timestamp)
{
    if (type == RTMP_PACKET_TYPE_VIDEO)
    {
        SIMPLE_LOG("send timestamp: %d \n", timestamp);
    }
    else
    {
        SIMPLE_LOG("send audio timestamp: %d\n", timestamp);
    }

    RTMPPacket rtmp_pakt;
    RTMPPacket_Reset(&rtmp_pakt);
    //RTMPPacket_Alloc(&rtmp_pakt, bufLen);
    rtmp_pakt.m_body = (char*)buf;
    rtmp_pakt.m_packetType = type;
    rtmp_pakt.m_nBodySize = bufLen;
    rtmp_pakt.m_nTimeStamp = timestamp;
    rtmp_pakt.m_nChannel = 4;
    rtmp_pakt.m_headerType = RTMP_PACKET_SIZE_LARGE;
    rtmp_pakt.m_nInfoField2 = rtmp_->m_stream_id;
    
    //memcpy(rtmp_pakt.m_body, buf, bufLen);

    int retval = RTMP_SendPacket(rtmp_, &rtmp_pakt, 0);
    //RTMPPacket_Free(&rtmp_pakt);

    return !!retval;
}

bool LibRtmp::CopySend(const char* buf, int bufLen, int type, unsigned int timestamp)
{
    SIMPLE_LOG("send timestamp: %d\n", timestamp);

    RTMPPacket rtmp_pakt;
    RTMPPacket_Reset(&rtmp_pakt);
    RTMPPacket_Alloc(&rtmp_pakt, bufLen);

    rtmp_pakt.m_packetType = type;
    rtmp_pakt.m_nBodySize = bufLen;
    rtmp_pakt.m_nTimeStamp = timestamp;
    rtmp_pakt.m_nChannel = 4;
    rtmp_pakt.m_headerType = RTMP_PACKET_SIZE_LARGE;
    rtmp_pakt.m_nInfoField2 = rtmp_->m_stream_id;

    memcpy(rtmp_pakt.m_body, buf, bufLen);

    int retval = RTMP_SendPacket(rtmp_, &rtmp_pakt, 0);
    RTMPPacket_Free(&rtmp_pakt);

    return !!retval;
}

void LibRtmp::SendSetChunkSize(unsigned int chunkSize)
{
    RTMPPacket rtmp_pakt;
    RTMPPacket_Reset(&rtmp_pakt);
    RTMPPacket_Alloc(&rtmp_pakt, 4);

    rtmp_pakt.m_packetType = 0x01;
    rtmp_pakt.m_nChannel = 0x02;    // control channel
    rtmp_pakt.m_headerType = RTMP_PACKET_SIZE_LARGE;
    rtmp_pakt.m_nInfoField2 = 0;


    rtmp_pakt.m_nBodySize = 4;
    UI32ToBytes(rtmp_pakt.m_body, chunkSize);

    RTMP_SendPacket(rtmp_, &rtmp_pakt, 0);
    RTMPPacket_Free(&rtmp_pakt);
}

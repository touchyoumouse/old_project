#include "stdafx.h"
#include "LibRtmp.h"

#include "AmfByteStream.h"

LibRtmp::LibRtmp(bool isNeedLog, bool isNeedRecord)
{
    if (isNeedLog)
    {
        flog_ = fopen("librtmp.log", "w");
        RTMP_LogSetLevel(RTMP_LOGDEBUG2);
        RTMP_LogSetOutput(flog_);
    }
    else
    {
        flog_ = NULL;
    }
	{
		/*WORD version;
		WSADATA wsaData;
		version=MAKEWORD(2,2);
		(WSAStartup(version, &wsaData) == 0);*/
	}
    rtmp_ = RTMP_Alloc();
    RTMP_Init(rtmp_);
	rtmp_->Link.timeout=5;	
	
	
    RTMP_SetBufferMS(rtmp_, 300);

    streming_url_ = NULL;
    is_need_record_ = isNeedRecord;
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

    std::string tmp(url);
    stream_name_ = tmp.substr(tmp.rfind("/")+1);
	int _blank_index = stream_name_.find(" ");
	stream_name_ = stream_name_.substr(0,_blank_index);
    //AVal flashver = AVC("flashver");
    //AVal flashver_arg = AVC("WIN 9,0,115,0");
    AVal swfUrl = AVC("swfUrl");
    AVal swfUrl_arg = AVC("http://localhost/librtmp.swf");
    AVal pageUrl = AVC("pageUrl");
    AVal pageUrl_arg = AVC("http://localhost/librtmp.html");
    //RTMP_SetOpt(rtmp_, &flashver, &flashver_arg);
    RTMP_SetOpt(rtmp_, &swfUrl, &swfUrl_arg);
    RTMP_SetOpt(rtmp_, &pageUrl, &pageUrl_arg);

    if (is_need_record_)
    {
        AVal record = AVC("record");
        AVal record_arg = AVC("on");
        RTMP_SetOpt(rtmp_, &record, &record_arg);
    }

	if (RTMP_SetupURL(rtmp_,(char*)url) == FALSE)
	{
		RTMP_Free(rtmp_);
		return false;
	}

    RTMP_EnableWrite(rtmp_);                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       
    
	if (RTMP_Connect(rtmp_, NULL) == FALSE) 
	{
		RTMP_Free(rtmp_);
		return false;
	} 

	/*Á¬½ÓÁ÷*/
	if (RTMP_ConnectStream(rtmp_,0) == FALSE)
	{
		RTMP_Close(rtmp_);
		RTMP_Free(rtmp_);
		return false;
	}

    rtmp_->m_outChunkSize = 1024*1024;
    SendSetChunkSize(rtmp_->m_outChunkSize);

    return true;
}

void LibRtmp::Close()
{
    RTMP_Close(rtmp_);
}

void LibRtmp::Send(const char* buf, int bufLen, int type, unsigned int timestamp)
{
	//base::AutoLock alock(queue_lock_);
    RTMPPacket *rtmp_pakt = NULL;
	rtmp_pakt = (RTMPPacket*)malloc(sizeof(RTMPPacket));
	RTMPPacket_Alloc(rtmp_pakt, bufLen);
    RTMPPacket_Reset(rtmp_pakt);
    

    rtmp_pakt->m_packetType = type;
    rtmp_pakt->m_nBodySize = bufLen;
    rtmp_pakt->m_nTimeStamp = timestamp;
    rtmp_pakt->m_nChannel = 0x04;
    rtmp_pakt->m_headerType = RTMP_PACKET_SIZE_LARGE;
    rtmp_pakt->m_nInfoField2 = rtmp_->m_stream_id;
    memcpy(rtmp_pakt->m_body, buf, bufLen);

	if (!RTMP_IsConnected(rtmp_)){
		RTMP_Log(RTMP_LOGERROR,"rtmp is not connect\n");
		
	}
	if (!RTMP_SendPacket(rtmp_,rtmp_pakt,0)){
		RTMP_Log(RTMP_LOGERROR,"Send Error\n");
	
	}
    RTMPPacket_Free(rtmp_pakt);
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

void LibRtmp::CreateSharedObject()
{
    char data_buf[4096];
    char* pbuf = data_buf;

    pbuf = AmfStringToBytes(pbuf, stream_name_.c_str());

    pbuf = UI32ToBytes(pbuf, 0);    // version
    pbuf = UI32ToBytes(pbuf, 0);    // persistent
    pbuf += 4;

    pbuf = UI08ToBytes(pbuf, RTMP_SHARED_OBJECT_DATATYPE_CONNECT);

    char* pbuf_datalen = pbuf;
    pbuf += 4;

    UI32ToBytes(pbuf_datalen, (int)(pbuf - pbuf_datalen - 4));

    int buflen = (int)(pbuf - data_buf);

    LibRtmp::Send(data_buf, buflen, TAG_TYPE_SHARED_OBJECT, 0);
}

void LibRtmp::SetSharedObject(bool isSet)
{
    char data_buf[4096];
    char* pbuf = data_buf;

    pbuf = AmfStringToBytes(pbuf, stream_name_.c_str());

    pbuf = UI32ToBytes(pbuf, 0);    // version
    pbuf = UI32ToBytes(pbuf, 0);    // persistent
    pbuf += 4;

    pbuf = UI08ToBytes(pbuf, RTMP_SHARED_OBJECT_DATATYPE_SET_ATTRIBUTE);

    char* pbuf_datalen = pbuf;
    pbuf += 4;

    pbuf = AmfStringToBytes(pbuf, "video_change");
    pbuf = AmfBoolToBytes(pbuf, isSet);
    UI32ToBytes(pbuf_datalen, (int)(pbuf - pbuf_datalen - 4));

    int buflen = (int)(pbuf - data_buf);

    LibRtmp::Send(data_buf, buflen, TAG_TYPE_SHARED_OBJECT, 0);
}

void LibRtmp::SendSharedObject(int val)
{
    char data_buf[4096];
    char* pbuf = data_buf;

    pbuf = AmfStringToBytes(pbuf, stream_name_.c_str());

    pbuf = UI32ToBytes(pbuf, 0);    // version
    pbuf = UI32ToBytes(pbuf, 0);    // persistent
    pbuf += 4;

    pbuf = UI08ToBytes(pbuf, RTMP_SHARED_OBJECT_DATATYPE_SET_ATTRIBUTE);

    char* pbuf_datalen = pbuf;
    pbuf += 4;

    pbuf = AmfStringToBytes(pbuf, "currentPage");
    pbuf = AmfDoubleToBytes(pbuf, val);
    UI32ToBytes(pbuf_datalen, (int)(pbuf - pbuf_datalen - 4));

    int buflen = (int)(pbuf - data_buf);

    LibRtmp::Send(data_buf, buflen, TAG_TYPE_SHARED_OBJECT, 0);
}

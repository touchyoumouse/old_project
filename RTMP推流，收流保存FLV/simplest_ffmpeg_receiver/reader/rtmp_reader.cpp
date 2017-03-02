/*******************************************************************************
 * rtmp_reader.cpp
 * Copyright: (c) 2014 Haibin Du(haibindev.cnblogs.com). All rights reserved.
 * -----------------------------------------------------------------------------
 *
 * -----------------------------------------------------------------------------
 * 2014-12-11 16:39 - Created (Haibin Du)
 ******************************************************************************/

#include "rtmp_reader.h"

#include <cassert>

#include "../rtmp_base/librtmp/log.h"

#include "../base/BitWritter.h"
#include "../base/bit_stream.h"
#include "../base/byte_stream.h"
#include "../base/h264_frame_parser.h"

static const int kSampleRateIndexs[16] = {
    96000, 88200, 64000, 48000, 44100, 32000,
    24000, 22050, 16000, 12000, 11025, 8000, 7350
};

RtmpReader::RtmpReader(bool isNeedLog)
{
    //hbase::EnsureWinsockInit();
    isNeedLog = false;

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

    rtmp_ = RTMP_Alloc();
    RTMP_Init(rtmp_);
    RTMP_SetBufferMS(rtmp_, 60*1000);
    rtmp_->Link.lFlags |= RTMP_LF_LIVE;
    rtmp_->Link.timeout = 5;

    streming_url_ = NULL;
    observer_ = NULL;

    is_stopping_ = false;
    connect_state_ = kRtmpInit;

    video_width_ = 0;
    video_height_ = 0;
    is_get_keyframe_ = false;

    samplerate_ = 0;
    channel_ = 0;

    is_codec_callback_ = false;
    is_codec_error_ = false;

    audio_timestamp_ = 0;
    video_timestamp_ = 0;
}

RtmpReader::~RtmpReader()
{

}

bool RtmpReader::Start(const char* url)
{
    streming_url_ = (char*)calloc(strlen(url)+1, sizeof(char));
    strcpy(streming_url_, url);

    AVal flashver = AVC("flashVer");
    AVal flashver_arg = AVC("WIN 9,0,115,0");
    AVal swfUrl = AVC("swfUrl");
    AVal swfUrl_arg = AVC("http://localhost/librtmp.swf");
    AVal pageUrl = AVC("pageUrl");
    AVal pageUrl_arg = AVC("http://localhost/librtmp.html");
    RTMP_SetOpt(rtmp_, &flashver, &flashver_arg);
    RTMP_SetOpt(rtmp_, &swfUrl, &swfUrl_arg);
    RTMP_SetOpt(rtmp_, &pageUrl, &pageUrl_arg);

    int err = RTMP_SetupURL(rtmp_, streming_url_);
    if (err <= 0) return false;

    ThreadStart();

    return true;
}

void RtmpReader::Stop()
{
    if (is_stopping_) return;

    is_stopping_ = true;

    // 如果还在连接，或者连接超时了
    if (connect_state_ == kRtmpConnecting ||
        (connect_state_ == kRtmpConnectSuccess &&
        recv_timecounter_.LastTimestamp() > 0 && recv_timecounter_.Get() >= 3000))
    {
        ThreadStop();
        //Close();
        closesocket(rtmp_->m_sb.sb_socket);
        ThreadJoin();
        return;
    }

    ThreadStop();
    if (connect_state_ != kRtmpConnectFailed)
    {
        ThreadJoin();
    }

    Close();
}

bool RtmpReader::Connect(const char* url)
{
    connect_state_ = kRtmpConnecting;

    int err = RTMP_Connect(rtmp_, NULL);
    if (err <= 0) return false;

    err = RTMP_ConnectStream(rtmp_, 0);
    if (err <= 0) return false;

    return true;
}

void RtmpReader::Close()
{
    if (rtmp_ == NULL) return;

    RTMP_Close(rtmp_);
    RTMP_Free(rtmp_);
    rtmp_ = NULL;

    if (streming_url_)
    {
        free(streming_url_);
        streming_url_ = NULL;
    }

    if (flog_) fclose(flog_);
}

void RtmpReader::SetObserver(Observer* ob)
{
    observer_ = ob;
}

bool RtmpReader::IsNeedReConnected()
{
    if (is_stopping_)
    {
        return false;
    }

    if (connect_state_ == kRtmpConnectFailed ||
        connect_state_ == kRtmpRecvFailed)
    {
        return true;
    }
    return false;
}

void RtmpReader::ThreadRun()
{
    if (false == Connect(streming_url_))
    {
        //is_stopping_ = true;

        connect_state_ = kRtmpConnectFailed;
        if (observer_)
            observer_->OnRtmpErrCode(kRtmpConnectFailed);
        return;
    }

    connect_state_ = kRtmpConnectSuccess;
    if (observer_)
        observer_->OnRtmpErrCode(kRtmpConnectSuccess);

    recv_timecounter_.Reset();

    for (;;)
    {
        if (is_stopping_)
            break;

        RTMPPacket rtmp_pakt = {0};
        int retval = RTMP_GetNextMediaPacket(rtmp_, &rtmp_pakt);
        if (retval <= 0)
        {
            connect_state_ = kRtmpRecvFailed;
            if (observer_)
                observer_->OnRtmpErrCode(kRtmpRecvFailed);

            break;
        }
        recv_timecounter_.Reset();

        //if (rtmp_pakt.m_hasAbsTimestamp)
        ////if (1)
        //{
        //    timestamp_ = rtmp_pakt.m_nTimeStamp;
        //}
        //else
        //{
        //    timestamp_ += rtmp_pakt.m_nTimeStamp;
        //}

        const char* buf = rtmp_pakt.m_body;
        unsigned int buf_len = rtmp_pakt.m_nBodySize;

        if (buf_len == 0) continue;

        if (rtmp_pakt.m_packetType == RTMP_PACKET_TYPE_VIDEO)
        {
            video_timestamp_ = rtmp_pakt.m_nTimeStamp;

            ParseVideoPacket(buf, buf_len);
        }
        else if (rtmp_pakt.m_packetType == RTMP_PACKET_TYPE_AUDIO)
        {
            audio_timestamp_ = rtmp_pakt.m_nTimeStamp;

            ParseAudioPacket(buf, buf_len);
        }
        else if (rtmp_pakt.m_packetType == RTMP_PACKET_TYPE_FLASH_VIDEO)
        {
            ParseFlashTag(buf, buf_len);
        }
        else if (rtmp_pakt.m_packetType == RTMP_PACKET_TYPE_INFO)   // 0x12
        {

        }
        RTMPPacket_Free(&rtmp_pakt);
    }
}

static char xxxxcode[] = {0x00, 0x00, 0x00, 0x01};

void RtmpReader::ParseVideoPacket(const char* dataBuf, unsigned int dataLen)
{
    if (dataLen <= 2) return;

    const char* pbuf = dataBuf;
    unsigned char flag = BytesToUI08(pbuf); pbuf += 1;
    
    unsigned int frame_type = (flag >> 4) & 0x0f;
    unsigned int codec_id   = flag & 0x0f;

    if (codec_id != 7)
    {
        is_codec_error_ = true;

        assert(0);
        return;
    }

    unsigned int avc_type = BytesToUI08(pbuf); pbuf += 1;
    pbuf += 3;

    unsigned int remain_len = dataBuf+dataLen-pbuf;
    if (avc_type == 0)      // seq header
    {
        ReadH264SequenceHeader(pbuf, remain_len);
    }
    else if (avc_type == 1) // NALUs
    {
        ReadH264Nalus(pbuf, remain_len);
    }
}

void RtmpReader::ReadH264SequenceHeader(const char* pbuf, 
    unsigned int buflen)
{
    if (video_width_==0 || video_height_==0)
    {
        pbuf += 6;

        unsigned int sps_size = BytesToUI16(pbuf); pbuf += 2;
        const char* sps_buf = pbuf;

//         h264_decode_seq_parameter_set((char*)pbuf, sps_size, 
//             video_width_, video_height_);
        ff_h264_decode_sps(pbuf, sps_size, &video_width_, &video_height_);

        if (video_width_ > 0 && video_height_ > 0 && observer_)
        {
            observer_->OnRtmpVideoCodec(video_width_, video_height_);
        }

        pbuf += sps_size;
        pbuf += 1;

        unsigned int pps_size = BytesToUI16(pbuf); pbuf += 2;
        const char* pps_buf = pbuf;
        
        char* tmpbuf = new char[sps_size+pps_size+8];
        memcpy(tmpbuf, xxxxcode, 4);
        memcpy(tmpbuf+4, sps_buf, sps_size);
        memcpy(tmpbuf+4+sps_size, xxxxcode, 4);
        memcpy(tmpbuf+4+sps_size+4, pps_buf, pps_size);
        if (observer_)
            observer_->OnRtmpVideoBuf(tmpbuf, sps_size+pps_size+8, video_timestamp_);
        delete[] tmpbuf;
    }
}

void RtmpReader::ReadH264Nalus(const char* pbuf, unsigned int buflen)
{
//     if (video_width_ > 0 && observer_ && !is_codec_callback_)
//     {
//         observer_->OnRtmpCodecInfo(samplerate_, channel_, video_width_, video_height_);
//         is_codec_callback_ = true;
//     }

    bool is_datanal = false;
    unsigned int remain_len = buflen;
    for (;;)
    {
        if (remain_len <= 4)
            break;

        unsigned int unit_size = BytesToUI32(pbuf);
        pbuf += 4; remain_len -= 4;
        if (unit_size > remain_len)
            break;

        unsigned char nal_type = pbuf[0] & 0x1f;
        if (nal_type == 5)
        {
            is_get_keyframe_ = true;
            is_datanal = true;
        }
        else if (nal_type == 7)
        {
            if (video_width_==0 || video_height_==0)
            {
                ff_h264_decode_sps(pbuf, unit_size,
                    &video_width_, &video_height_);
                if (video_width_ > 0 && video_height_ > 0 && observer_)
                {
                    observer_->OnRtmpVideoCodec(video_width_, video_height_);
                }
            }
        }
        else if (nal_type == 8)
        {
            int pps = 1;
        }
        else
        {
            is_datanal = true;
        }

        if (video_width_ && video_height_/* && is_datanal*/)
        {
            memcpy((char*)(pbuf-4), xxxxcode, 4);
            if (observer_)
                observer_->OnRtmpVideoBuf(pbuf-4, unit_size+4, video_timestamp_);
        }
        pbuf += unit_size; remain_len -= unit_size;
    }
}

void RtmpReader::ParseAudioPacket(const char* dataBuf, unsigned int dataLen)
{
    const char* pbuf = dataBuf;
    unsigned char flag = BytesToUI08(pbuf); pbuf += 1;

    unsigned int sound_format = (flag >> 4) & 0x0f;
    unsigned int sound_rate   = (flag >> 2) & 0x03;
    unsigned int sound_bits   = (flag >> 1) & 0x01;
    unsigned int sound_type   =  flag & 0x01;

    if (sound_format == 2)          // mp3
    {
		//@feng 断点何意？
       // assert(0);
    }
    else if (sound_format == 10)    // aac
    {
        unsigned int aac_type = BytesToUI08(pbuf); pbuf += 1;
        unsigned int remain_len = dataBuf+dataLen-pbuf;
        if (aac_type == 0)      // seq header
        {
            ReadAACSequenceHeader(pbuf, remain_len);
        }
        else if (aac_type == 1) // aac packets
        {
            ReadAACPackets(pbuf, remain_len);
        }
    }
    else if (sound_format == 11)    // speex
    {
        assert(0);
    }
    else
    {
        //assert(0);
    }
}

void RtmpReader::ReadAACSequenceHeader(const char* pbuf, 
    unsigned int buflen)
{
    BitReader bit_reader(pbuf);
    unsigned int object_type = bit_reader.ReadBitUI32(5);
    unsigned int sample_rate_index = bit_reader.ReadBitUI32(4);
    unsigned int channel = bit_reader.ReadBitUI32(4);
    // skip others

    samplerate_ = kSampleRateIndexs[sample_rate_index];
    channel_ = channel;

//     if (video_width_ > 0 && observer_)
//     {
//         observer_->OnRtmpCodecInfo(samplerate_, channel_,
//             video_width_, video_height_);
//         is_codec_callback_ = true;
//     }

    if (observer_)
        observer_->OnRtmpAudioCodec(samplerate_, channel_);
}

void RtmpReader::ReadAACPackets(const char* pbuf, unsigned int buflen)
{
    if (observer_)
        observer_->OnRtmpAudioBuf(pbuf, buflen, audio_timestamp_);
}

void RtmpReader::ParseFlashTag(const char* dataBuf, unsigned int dataLen)
{
    const char* ptag = dataBuf;
    int remain_len = dataLen;
    for (;;)
    {
        if (remain_len < 11)
            break;

        const char* pbuf = ptag;
        unsigned int tag_type =  BytesToUI08(pbuf); pbuf += 1;
        unsigned int data_size = BytesToUI24(pbuf); pbuf += 3;
        unsigned int timestamp = BytesToUI24(pbuf); pbuf += 3;
        timestamp |= (BytesToUI08(pbuf) << 24 ); pbuf += 1;
        unsigned int stream_id = BytesToUI24(pbuf); pbuf += 3;

        if (tag_type == RTMP_PACKET_TYPE_AUDIO)
        {
            audio_timestamp_ = timestamp;

            ParseAudioPacket(pbuf, data_size);
        }
        else if (tag_type == RTMP_PACKET_TYPE_VIDEO)
        {
            video_timestamp_ = timestamp;

            ParseVideoPacket(pbuf, data_size);
        }

        int total_size = 11 + data_size + 4;
        ptag += total_size;
        remain_len -= total_size;
    }
}

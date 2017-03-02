/*******************************************************************************
 * rtmp_publish.cpp
 * Copyright: (c) 2014 Haibin Du(haibindev.cnblogs.com). All rights reserved.
 * -----------------------------------------------------------------------------
 *
 * -----------------------------------------------------------------------------
 * 2015-3-17 16:11 - Created (Haibin Du)
 ******************************************************************************/

#include "rtmp_publish.h"

#if defined(WIN32)
#include <ObjBase.h>
#include <Shlwapi.h>
#endif

#include <algorithm>
#include <cstdlib>
#include <vector>

extern "C"
{
#include "libavutil/mathematics.h"
}

#include "base/AmfByteStream.h"
#include "base/byte_stream.h"
#include "base/BitWritter.h"
#include "base/h264_frame_parser.h"
#include "base/simple_logger.h"

#include "lib_rtmp.h"
#include "flv_writter.h"

RtmpPublish::RtmpPublish(
    const std::string& rtmpUrl, // 发布rtmp流的地址(rtmp://x.x.x.x/live/streamname)
    bool isNeedLog,             // 是否需要librtmp打印日志
    int audioSampleRate,        // 音频采样率(一般为44100 22050 16000)
    int audioChannels,          // 音频声道数(一般为1 2)
    int videoWidth,             // 视频宽
    int videoHeight             // 视频高
    )
{
    is_has_audio_ = true;
    is_has_video_ = true;

    source_samrate_ = audioSampleRate;
    source_channel_ = audioChannels;
    video_width_ = videoWidth;
    video_height_ = videoHeight;

    if (video_width_ == 0 || video_height_ == 0)
    {
        video_width_ = 1920;
        video_height_ = 1080;
    }

    rtmp_url_ = rtmpUrl;
    librtmp_ = new LibRtmp(isNeedLog);

    // 编码信息存储
    //x264_buf_ = new char[video_width_*video_height_*2];
    x264_len_ = 0;
    sps_ = new char[1024];
    sps_size_ = 0;
    pps_ = new char[1024];
    pps_size_ = 0;

    aac_info_buf_ = NULL;
    aac_info_size_ = 0;
    aac_frame_count_ = 0;

    // 录制
    flv_writter_ = NULL;
    record_interval_ = 0;
    last_record_hour_ = -1;

    // 其他信息
    pub_state_ = RTMP_PUB_INIT;
    is_started_ = false;    

    is_recording_ = false;

    has_flv_writter_header_ = false;
    is_header_send_ = false;
}

RtmpPublish::~RtmpPublish()
{
    delete librtmp_;

    delete[] aac_info_buf_;
    //delete[] x264_buf_;
    delete[] sps_;
    delete[] pps_;
}

void RtmpPublish::Start()
{
    ThreadStart();
}

void RtmpPublish::Stop()
{
    if (IsThreadStop()) return;

    ThreadStop();

    // 如果还在连接，或者连接超时了
    if (pub_state_ == RTMP_PUB_CONNECTING)
    {
        closesocket(librtmp_->GetRTMP()->m_sb.sb_socket);
        ThreadJoin();
        return;
    }

    if (pub_state_ != RTMP_PUB_CONNECT_FAILED)
    {
        ThreadJoin();
    }

    for (std::map<RtmpBuffer*, bool>::iterator it = buf_memory_caches_.begin();
        it != buf_memory_caches_.end(); ++it)
    {
        RtmpBuffer* cache = it->first;
        delete cache;
    }
}

void RtmpPublish::ThreadRun()
{
#ifdef WIN32
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    WORD version;  
    WSADATA wsaData;  
    version = MAKEWORD(1, 1);
    WSAStartup(version, &wsaData);
#endif

    SIMPLE_LOG("opening\n");

    pub_state_ = RTMP_PUB_CONNECTING;

    // 连接rtmp server，完成握手等协议
    bool is_ok = librtmp_->Open(rtmp_url_.c_str());
    if (IsThreadStop()) return;

    if (false == is_ok)
    {
        SIMPLE_LOG("open failed\n");

        pub_state_ = RTMP_PUB_CONNECT_FAILED;

        ThreadStop();
        return;
    }

    pub_state_ = RTMP_PUB_CONNECT_SUCCEED;

    // 发送metadata包
    //SendMetadataPacket();

    SIMPLE_LOG("try sending header\n");

    TrySendSequenceHeader();

    is_started_ = true;

    while (true)
    {
        if (base::SimpleThread::IsThreadStop()) break;

        // 检查队列长度，从队列中取出数据
        std::deque<RtmpBuffer*> rtmp_datas;
        {
            base::AutoLock al(list_mtx_);

            CheckBuflist_Locked();
            rtmp_datas.swap(encode_buflist_);
        }

        //SIMPLE_LOG("rtmp data size: %d\n", rtmp_datas.size());

        // 加上时戳，发送给rtmp server
        for (std::deque<RtmpBuffer*>::iterator it = rtmp_datas.begin();
            it != rtmp_datas.end(); ++it)
        {
            if (base::SimpleThread::IsThreadStop())
            {
                SIMPLE_LOG("we need break\n");
                break;
            }

            RtmpBuffer* rtmp_data = *it;
            if (rtmp_data != NULL)
            {
                if (rtmp_data->type_ == RtmpBuffer::RTMP_DATA_AUDIO)
                {
                    ProcessAACBuffer(rtmp_data->DataBuf()+RTMP_MAX_HEADER_SIZE, rtmp_data->DataSize(),
                        rtmp_data->timestamp_);
                }
                else if (rtmp_data->type_ == RtmpBuffer::RTMP_DATA_VIDEO)
                {
                    ProcessH264Buffer(rtmp_data->DataBuf()+RTMP_MAX_HEADER_SIZE, rtmp_data->DataSize(),
                        rtmp_data->timestamp_, rtmp_data->is_keyframe_);
                }
            }
        }

        // delete rtmp buf
        for (std::deque<RtmpBuffer*>::iterator it = rtmp_datas.begin();
            it != rtmp_datas.end(); ++it)
        {
            RtmpBuffer* rtmp_data = *it;
            base::AutoLock al(list_mtx_);
            DeleteRtmpBuf(rtmp_data);
        }

        if (rtmp_datas.empty())
        {
            MillsecSleep(10);
        }
    }

    SIMPLE_LOG("rtmp thread end\n");

    librtmp_->Close();

    SIMPLE_LOG("librtmp closed\n");

#ifdef WIN32
    WSACleanup();

    CoUninitialize();
#endif
}

void RtmpPublish::ProcessAACBuffer(char* dataBuf, int dataSize,
    long long timestamp)
{
    SendAudioData(dataBuf, dataSize, timestamp);

    if (is_recording_ && flv_writter_ && has_flv_writter_header_)
    {
        flv_writter_->WriteAudioDataTag(dataBuf, dataSize, timestamp);
    }
}

void RtmpPublish::ProcessH264Buffer(char* dataBuf, int dataSize,
    long long timestamp, bool isKeyframe)
{
    SendVideoData(dataBuf, dataSize, timestamp, isKeyframe);

    if (is_recording_)
    {
        // 检测是否需要新建文件
        SYSTEMTIME time;
        GetLocalTime(&time);
        if (IsNeedNextRecord(time.wHour))
        {
            last_record_hour_ = time.wHour;
            if (flv_writter_)
            {
                flv_writter_->Close();
                delete flv_writter_;
            }
            flv_writter_ = new FlvWritter();
            std::string flvname = GetRecordFilename(time.wYear, time.wMonth, time.wDay, time.wHour);
            flv_writter_->Open(flvname.c_str());
            flv_writter_->WriteFlvHeader(is_has_audio_, is_has_video_);

            if (sps_size_ && pps_size_)
            {
                flv_writter_->WriteAVCSequenceHeaderTag(sps_, sps_size_, pps_, pps_size_);
            }
            if (source_samrate_ && source_channel_)
            {
                flv_writter_->WriteAACSequenceHeaderTag(source_samrate_, source_channel_);
            }
        }

        if (flv_writter_ && has_flv_writter_header_)
        {
            flv_writter_->WriteVideoDataTag(dataBuf, dataSize, timestamp);
        }
    }
}

void RtmpPublish::TrySendSequenceHeader()
{
    if (is_has_video_)
    {
        if (sps_size_ && pps_size_)
        {
            if (is_has_video_) SendAVCSequenceHeaderPacket();
            if (is_has_audio_) SendAACSequenceHeaderPacket();

            timestamp_.Reset();

            is_header_send_ = true;
        }
    }
    else    // 只有音频
    {
        if (is_has_audio_) SendAACSequenceHeaderPacket();

        timestamp_.Reset();
        is_header_send_ = true;
    }
}

void RtmpPublish::CheckBuflist_Locked()
{
    if (encode_buflist_.size() >= 100)
    {
        bool is_first_unit = true;
        while (false == encode_buflist_.empty())
        {
            RtmpBuffer* rtmp_data = encode_buflist_.front();
            if (rtmp_data->is_keyframe_ && !is_first_unit)
            {
                break;
            }

            DeleteRtmpBuf(rtmp_data);

            encode_buflist_.pop_front();
            is_first_unit = false;
        }
    }
}

void RtmpPublish::SetAACSpecificInfo(const char* infoBuf, int bufSize)
{
    base::AutoLock al(list_mtx_);

    if (SimpleThread::IsThreadStop()) return;

    aac_info_buf_ = new char[bufSize];
    memcpy(aac_info_buf_, infoBuf, bufSize);
    aac_info_size_ = bufSize;
}

void RtmpPublish::PostAACFrame(const char* buf, int bufLen)
{
    base::AutoLock al(list_mtx_);

    if (SimpleThread::IsThreadStop()) return;

    long long audio_pts = av_rescale(aac_frame_count_, 1000, source_samrate_);
    //long long audio_pts = aac_frame_count_ * 1024 * 1000 / source_samrate_;
    aac_frame_count_ += 1024;

    if ( (false == is_has_video_) && (false == is_header_send_) )
    {
        if (is_started_)
        {
            if (is_has_audio_) SendAACSequenceHeaderPacket();

            is_header_send_ = true;
        }
    }

    if (false == is_header_send_) return;

    if (timestamp_.LastTimestamp() == -1)
    {
        timestamp_.Reset();
    }

    RtmpBuffer* audiobuf = CreateRtmpBuf(bufLen+2+RTMP_MAX_HEADER_SIZE, false);
    audiobuf->type_ = RtmpBuffer::RTMP_DATA_AUDIO;
    audiobuf->timestamp_ = timestamp_.Get();
    audiobuf->is_keyframe_ = false;

    audiobuf->timestamp_ = audio_pts;

    memcpy(audiobuf->DataBuf()+2+RTMP_MAX_HEADER_SIZE, buf, bufLen);
    audiobuf->SetSize(bufLen+2);

#if 1
    ProcessAACBuffer(audiobuf->DataBuf()+RTMP_MAX_HEADER_SIZE, audiobuf->DataSize(),
        audiobuf->timestamp_);
    DeleteRtmpBuf(audiobuf);
#else
    encode_buflist_.push_back(audiobuf);
#endif
}

void RtmpPublish::PostH264Frame(const char* buf, int bufLen, long long shouldTS)
{
    base::AutoLock al(list_mtx_);

    if (SimpleThread::IsThreadStop()) return;

    if (!is_started_) return;

//     while (!is_started_)
//     {
//         if (SimpleThread::IsThreadStop()) return;
//         MillsecSleep(10);
//     }

    bool is_keyframe = false;
    bool is_first = false;

    RtmpBuffer* videobuf = CreateRtmpBuf(bufLen+1024+RTMP_MAX_HEADER_SIZE, true);
    videobuf->type_ = RtmpBuffer::RTMP_DATA_VIDEO;

    if (sps_size_ == 0 || pps_size_ == 0)
    {
        is_first = true;
        ParseH264Frame(buf, bufLen, videobuf->DataBuf()+5+RTMP_MAX_HEADER_SIZE, x264_len_,
            sps_, sps_size_, pps_, pps_size_, is_keyframe, NULL, NULL);
    }
    else
    {
        ParseH264Frame(buf, bufLen, videobuf->DataBuf()+5+RTMP_MAX_HEADER_SIZE, x264_len_,
            NULL, sps_size_, NULL, pps_size_, is_keyframe, NULL, NULL);
    }

    if (is_first && sps_size_ && pps_size_)
    {
        OnSPSAndPPS(sps_, sps_size_, pps_, pps_size_);
    }

    if (x264_len_ > 0 && sps_size_ && pps_size_)
    {
        if (timestamp_.LastTimestamp() == -1)
        {
            timestamp_.Reset();
        }

        videobuf->SetSize(x264_len_+5);
        videobuf->is_keyframe_ = is_keyframe;
        videobuf->timestamp_ = timestamp_.Get();
        //videobuf->timestamp_ = shouldTS;

#if 1
        ProcessH264Buffer(videobuf->DataBuf()+RTMP_MAX_HEADER_SIZE, videobuf->DataSize(),
            videobuf->timestamp_, videobuf->is_keyframe_);
        DeleteRtmpBuf(videobuf);
#else
        encode_buflist_.push_back(videobuf);
#endif
    }
    else
    {
        DeleteRtmpBuf(videobuf);
    }
}

void RtmpPublish::SendVideoData(char* buf, int bufLen, unsigned int timestamp, bool isKeyframe)
{
    //FILE_LOG_DEBUG("size: %d, timestamp: %d\n", bufLen, timestamp);

    char* pbuf = buf;

    unsigned char flag = 0;
    if (isKeyframe)
        flag = 0x17;
    else
        flag = 0x27;

    pbuf = UI08ToBytes(pbuf, flag);

    pbuf = UI08ToBytes(pbuf, 1);    // avc packet type (0, nalu)
    pbuf = UI24ToBytes(pbuf, 0);    // composition time

    bool isok = librtmp_->Send(buf, bufLen, FLV_TAG_TYPE_VIDEO, timestamp);
    if (false == isok)
    {
        pub_state_ = RTMP_PUB_SEND_FAILED;
    }
}

void RtmpPublish::SendAudioData(char* buf, int bufLen, unsigned int timestamp)
{
    //FILE_LOG_DEBUG("size: %d, timestamp: %d\n", bufLen, timestamp);

    char* pbuf = buf;

    unsigned char flag = 0;

    flag = (10 << 4) |  // soundformat "10 == AAC"
        (3 << 2) |      // soundrate   "3  == 44-kHZ"
        (1 << 1) |      // soundsize   "1  == 16bit"
        1;              // soundtype   "1  == Stereo"

    pbuf = UI08ToBytes(pbuf, flag);

    pbuf = UI08ToBytes(pbuf, 1);    // aac packet type (1, raw)

    bool isok = librtmp_->Send(buf, bufLen, FLV_TAG_TYPE_AUDIO, timestamp);
    if (false == isok)
    {
        pub_state_ = RTMP_PUB_SEND_FAILED;
    }
}

void RtmpPublish::SendMetadataPacket()
{
    char metadata_buf[4096];

    char* pbuf = metadata_buf;

    pbuf = UI08ToBytes(pbuf, AMF_DATA_TYPE_STRING);
    pbuf = AmfStringToBytes(pbuf, "@setDataFrame");

    pbuf = UI08ToBytes(pbuf, AMF_DATA_TYPE_STRING);
    pbuf = AmfStringToBytes(pbuf, "onMetaData");

    //     pbuf = UI08ToBytes(pbuf, AMF_DATA_TYPE_MIXEDARRAY);
    //     pbuf = UI32ToBytes(pbuf, 2);
    pbuf = UI08ToBytes(pbuf, AMF_DATA_TYPE_OBJECT);

    //     pbuf = AmfStringToBytes(pbuf, "width");
    //     pbuf = AmfDoubleToBytes(pbuf, width_);
    // 
    //     pbuf = AmfStringToBytes(pbuf, "height");
    //     pbuf = AmfDoubleToBytes(pbuf, height_);

    pbuf = AmfStringToBytes(pbuf, "framerate");
    pbuf = AmfDoubleToBytes(pbuf, 10);

    pbuf = AmfStringToBytes(pbuf, "videocodecid");
    pbuf = UI08ToBytes(pbuf, AMF_DATA_TYPE_STRING);
    pbuf = AmfStringToBytes(pbuf, "avc1");

    // 0x00 0x00 0x09
    pbuf = AmfStringToBytes(pbuf, "");
    pbuf = UI08ToBytes(pbuf, AMF_DATA_TYPE_OBJECT_END);

    librtmp_->Send(metadata_buf, (int)(pbuf - metadata_buf), FLV_TAG_TYPE_META, 0);
}

void RtmpPublish::SendAVCSequenceHeaderPacket()
{
    char avc_seq_buf[4096];

    char* pbuf = avc_seq_buf;

    unsigned char flag = 0;
    flag = (1 << 4) |   // frametype "1  == keyframe"
        7;              // codecid   "7  == AVC"

    pbuf = UI08ToBytes(pbuf, flag);

    pbuf = UI08ToBytes(pbuf, 0);    // avc packet type (0, header)
    pbuf = UI24ToBytes(pbuf, 0);    // composition time

    // AVCDecoderConfigurationRecord

    pbuf = UI08ToBytes(pbuf, 1);            // configurationVersion
    pbuf = UI08ToBytes(pbuf, sps_[1]);      // AVCProfileIndication
    pbuf = UI08ToBytes(pbuf, sps_[2]);      // profile_compatibility
    pbuf = UI08ToBytes(pbuf, sps_[3]);      // AVCLevelIndication
    pbuf = UI08ToBytes(pbuf, 0xff);         // 6 bits reserved (111111) + 2 bits nal size length - 1
    pbuf = UI08ToBytes(pbuf, 0xe1);         // 3 bits reserved (111) + 5 bits number of sps (00001)
    pbuf = UI16ToBytes(pbuf, sps_size_);    // sps
    memcpy(pbuf, sps_, sps_size_);
    pbuf += sps_size_;
    pbuf = UI08ToBytes(pbuf, 1);            // number of pps
    pbuf = UI16ToBytes(pbuf, pps_size_);    // pps
    memcpy(pbuf, pps_, pps_size_);
    pbuf += pps_size_;

    librtmp_->CopySend(avc_seq_buf, (int)(pbuf - avc_seq_buf), FLV_TAG_TYPE_VIDEO, 0);
}

void RtmpPublish::SendAACSequenceHeaderPacket()
{
    char aac_seq_buf[4096];

    char* pbuf = aac_seq_buf;

    unsigned char flag = 0;
    flag = (10 << 4) |  // soundformat "10 == AAC"
        (3 << 2) |      // soundrate   "3  == 44-kHZ"
        (1 << 1) |      // soundsize   "1  == 16bit"
        1;              // soundtype   "1  == Stereo"

    pbuf = UI08ToBytes(pbuf, flag);

    pbuf = UI08ToBytes(pbuf, 0);    // aac packet type (0, header)

    // AudioSpecificConfig

    if (aac_info_size_ > 0)
    {
        memcpy(pbuf, aac_info_buf_, aac_info_size_);
        pbuf += aac_info_size_;
    }
    else
    {
        int sample_rate_index;
        if (source_samrate_ == 48000)
            sample_rate_index = 0x03;
        else if (source_samrate_ == 44100)
            sample_rate_index = 0x04;
        else if (source_samrate_ == 32000)
            sample_rate_index = 0x05;
        else if (source_samrate_ == 24000)
            sample_rate_index = 0x06;
        else if (source_samrate_ == 22050)
            sample_rate_index = 0x07;
        else if (source_samrate_ == 16000)
            sample_rate_index = 0x08;
        else if (source_samrate_ == 12000)
            sample_rate_index = 0x09;
        else if (source_samrate_ == 11025)
            sample_rate_index = 0x0a;
        else if (source_samrate_ == 8000)
            sample_rate_index = 0x0b;

        PutBitContext pb;
        init_put_bits(&pb, pbuf, 1024);
        put_bits(&pb, 5, 2);    //object type - AAC-LC
        put_bits(&pb, 4, sample_rate_index); // sample rate index
        put_bits(&pb, 4, source_channel_);    // channel configuration
        //GASpecificConfig
        put_bits(&pb, 1, 0);    // frame length - 1024 samples
        put_bits(&pb, 1, 0);    // does not depend on core coder
        put_bits(&pb, 1, 0);    // is not extension

        flush_put_bits(&pb);

        pbuf += 2;
    }

    librtmp_->CopySend(aac_seq_buf, (int)(pbuf - aac_seq_buf), FLV_TAG_TYPE_AUDIO, 0);
}

// 当收到sps和pps信息时，发送AVC和AAC的sequence header
void RtmpPublish::OnSPSAndPPS(char* spsBuf, int spsSize, char* ppsBuf, int ppsSize)
{
    memcpy(sps_, spsBuf, spsSize);
    sps_size_ = spsSize;

    memcpy(pps_, ppsBuf, ppsSize);
    pps_size_ = ppsSize;

    if (is_started_)
    {
        if (is_has_audio_) SendAACSequenceHeaderPacket();
        if (is_has_video_) SendAVCSequenceHeaderPacket();

        timestamp_.Reset();

        is_header_send_ = true;
    }

    if (flv_writter_)
    {
        if (is_has_audio_) flv_writter_->WriteAACSequenceHeaderTag(source_samrate_, source_channel_);
        if (is_has_video_) flv_writter_->WriteAVCSequenceHeaderTag(sps_, sps_size_, pps_, pps_size_);

        has_flv_writter_header_ = true;
    }
}

void RtmpPublish::ClearSpsAndPps()
{
    sps_size_ = 0;
    pps_size_ = 0;
}

void RtmpPublish::SetRecord(const std::string& recordDir, int recordInterval)
{
    base::AutoLock al(list_mtx_);

    record_dir_ = recordDir;
    record_interval_ = recordInterval;
}

void RtmpPublish::StartRecord()
{
    base::AutoLock al(list_mtx_);

    if (flv_writter_)
    {
        flv_writter_->Close();
        delete flv_writter_;
        has_flv_writter_header_ = false;
    }

    SYSTEMTIME time;
    GetLocalTime(&time);
    std::string flvname = GetRecordFilename(time.wYear, time.wMonth, time.wDay, time.wHour);

    flv_writter_ = new FlvWritter();
    if (false == flv_writter_->Open(flvname.c_str()))
    {
        delete flv_writter_;
        flv_writter_ = NULL;
        return;
    }

    flv_writter_->WriteFlvHeader(is_has_audio_, is_has_video_);

    last_record_hour_ = time.wHour;

    if (is_has_video_)
    {
        if (sps_size_ && pps_size_)
        {
            if (is_has_audio_) flv_writter_->WriteAACSequenceHeaderTag(source_samrate_, source_channel_);
            if (is_has_video_) flv_writter_->WriteAVCSequenceHeaderTag(sps_, sps_size_, pps_, pps_size_);

            has_flv_writter_header_ = true;
        }
    }
    else    // 只有音频
    {
        if (is_has_audio_) flv_writter_->WriteAACSequenceHeaderTag(source_samrate_, source_channel_);
        has_flv_writter_header_ = true;
    }
    is_recording_ = true;
}

void RtmpPublish::StopRecord()
{
    base::AutoLock al(list_mtx_);

    if (flv_writter_)
    {
        flv_writter_->Close();
        delete flv_writter_;
        flv_writter_ = NULL;
        has_flv_writter_header_ = false;
    }
    is_recording_ = false;
}

static void MakeAllDir(const std::string path)
{
#ifdef WIN32
    if (::PathFileExistsA(path.c_str())) return;

    if (!::CreateDirectoryA(path.c_str(), NULL))
    {
        std::string::size_type pos = path.rfind('\\');
        std::string parent_path = path.substr(0, pos);
        MakeAllDir(parent_path);
        ::CreateDirectoryA(path.c_str(), NULL);
    }
#else
    if (access(path.c_str(), 0) == 0) return;

    if (mkdir(path.c_str(), 0755) != 0)
    {
        std::string::size_type pos = path.rfind('/');
        std::string parent_path = path.substr(0, pos);
        MakeAllDir(parent_path);
        mkdir(path.c_str(), 0755);
    }
#endif
}

std::string RtmpPublish::GetRecordFilename(int year, int month, int day, int hour)
{
    return record_dir_;

    char buf[2048];
    sprintf(buf, "%s%d%02d\\%02d", record_dir_.c_str(), year, month, day);
    MakeAllDir(buf);

    sprintf(buf, "%s%d%02d\\%02d\\%2d.flv", record_dir_.c_str(), year, month, day, hour);
    return buf;
}

bool RtmpPublish::IsNeedNextRecord(int nowHour)
{
    return false;
    if (is_recording_)
    {
        int next_record_hour = last_record_hour_ + record_interval_;
        if (next_record_hour >= 24)
        {
            next_record_hour -= 24;
        }
        return nowHour == next_record_hour;
    }
    return false;
}

bool RtmpPublish::IsNeedReConnect()
{
    if (pub_state_ == RTMP_PUB_CONNECT_FAILED ||
        pub_state_ == RTMP_PUB_SEND_FAILED)
    {
        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
// memory cache

#define USE_RTMP_BUF_CACHE 1
RtmpBuffer* RtmpPublish::CreateRtmpBuf(int dataSize, bool isVideo)
{
    RtmpBuffer* buf = NULL;

#if defined(USE_RTMP_BUF_CACHE)
    if (isVideo)
    {
        for (std::map<RtmpBuffer*, bool>::iterator it = buf_memory_caches_.begin();
            it != buf_memory_caches_.end(); ++it)
        {
            RtmpBuffer* cache = it->first;
            bool is_using = it->second;

            if (is_using) continue;

            cache->MakeSureCapacity(dataSize);        // use cache

            buf = cache;
            it->second = true;

            break;
        }

        if (buf == NULL)    // not find
        {
            buf = new RtmpBuffer(dataSize);

            buf_memory_caches_.insert(std::make_pair(buf, true));
        }
    }
    else
#endif
    {
        buf = new RtmpBuffer(dataSize);
    }

    return buf;
}

void RtmpPublish::DeleteRtmpBuf(RtmpBuffer* rtmpBuf)
{
    rtmpBuf->Clear();

#if defined(USE_RTMP_BUF_CACHE)
    if (rtmpBuf->type_ == RtmpBuffer::RTMP_DATA_VIDEO)
    {
        std::map<RtmpBuffer*, bool>::iterator it = buf_memory_caches_.find(rtmpBuf);
        if (it != buf_memory_caches_.end())
        {
            it->second = false;
        }
        else
        {
            delete rtmpBuf;
        }
    }
    else
#endif
    {
        delete rtmpBuf;
    }
}


#include "flv_writter.h"

#include <string.h>
#include <stdlib.h>

#include "base/AmfByteStream.h"
#include "base/byte_stream.h"
#include "base/BitWritter.h"

typedef struct 
{
    unsigned char type;
    unsigned char datasize[3];
    unsigned char timestamp[3];
    unsigned char timestamp_ex;
    unsigned char streamid[3];
} FlvTag;

const unsigned char kFlvFileHeaderData[] = "FLV\x1\x5\0\0\0\x9\0\0\0\0";

FlvWritter::FlvWritter()
{
    file_handle_ = NULL;
    time_begin_ = -1;

    audio_mem_buf_size_ = 0;
    audio_mem_buf_ = NULL;
    video_mem_buf_size_ = 0;
    video_mem_buf_ = NULL;

    last_pause_pts_ = -1;
    delayed_pts_ = 0;

    opening_pts_ = 0;

    last_video_pts_ = 0;
}

FlvWritter::~FlvWritter()
{
    Close();
}

bool FlvWritter::Open(const char* filename)
{
    filename_ = filename;

    file_handle_ = fopen(filename, "wb");
    if (!file_handle_)
    {
        return false;
    }
    return true;
}

void FlvWritter::Close()
{
    if (file_handle_)
    {
        fclose(file_handle_);
        file_handle_ = NULL;
    }

    if (audio_mem_buf_)
        delete[] audio_mem_buf_;
    if (video_mem_buf_)
        delete[] video_mem_buf_;
}

void FlvWritter::SetPause(bool isPause, long long timestamp)
{
    if (last_pause_pts_ >= 0)  // 暂停中
    {
        if (false == isPause)
        {
            // 取消暂停

            delayed_pts_ += (timestamp-last_pause_pts_);

            last_pause_pts_ = -1;
        }
    }
    else                       // 正常情况
    {
        if (isPause)
        {
            // 开始暂停

            last_pause_pts_ = timestamp;
        }
    }
}

void FlvWritter::WriteFlvHeader(bool isHaveAudio, bool isHaveVideo)
{
    static char flv_file_header[] = "FLV\x1\x5\0\0\0\x9\0\0\0\0";

    if (isHaveAudio && isHaveVideo)
        flv_file_header[4] = 0x05;
    else if (isHaveAudio && !isHaveVideo)
        flv_file_header[4] = 0x04;
    else if (!isHaveAudio && isHaveVideo)
        flv_file_header[4] = 0x01;
    else
        flv_file_header[4] = 0x00;

    fwrite(flv_file_header, 13, 1, file_handle_);
}

void FlvWritter::WriteFlvHeader(const char* headerBuf, int bufLen)
{
    fwrite(headerBuf, bufLen, 1, file_handle_);
}

void FlvWritter::WriteAACSequenceHeaderTag(
    int sampleRate, int channel)
{
    char aac_seq_buf[4096];
    char* pbuf = aac_seq_buf;
    PutBitContext pb;
    int sample_rate_index = 0x04;

    unsigned char flag = 0;
    flag = (10 << 4) |  // soundformat "10 == AAC"
        (3 << 2) |      // soundrate   "3  == 44-kHZ"
        (1 << 1) |      // soundsize   "1  == 16bit"
        1;              // soundtype   "1  == Stereo"

    pbuf = UI08ToBytes(pbuf, flag);

    pbuf = UI08ToBytes(pbuf, 0);    // aac packet type (0, header)

    // AudioSpecificConfig

    if (sampleRate == 48000)        sample_rate_index = 0x03;
    else if (sampleRate == 44100)   sample_rate_index = 0x04;
    else if (sampleRate == 32000)   sample_rate_index = 0x05;
    else if (sampleRate == 24000)   sample_rate_index = 0x06;
    else if (sampleRate == 22050)   sample_rate_index = 0x07;
    else if (sampleRate == 16000)   sample_rate_index = 0x08;
    else if (sampleRate == 12000)   sample_rate_index = 0x09;
    else if (sampleRate == 11025)   sample_rate_index = 0x0a;
    else if (sampleRate == 8000)    sample_rate_index = 0x0b;

    init_put_bits(&pb, pbuf, 1024);
    put_bits(&pb, 5, 2);    //object type - AAC-LC
    put_bits(&pb, 4, sample_rate_index); // sample rate index
    put_bits(&pb, 4, channel);    // channel configuration
    //GASpecificConfig
    put_bits(&pb, 1, 0);    // frame length - 1024 samples
    put_bits(&pb, 1, 0);    // does not depend on core coder
    put_bits(&pb, 1, 0);    // is not extension

    flush_put_bits(&pb);

    pbuf += 2;

    WriteAudioTag(aac_seq_buf, (int)(pbuf - aac_seq_buf), 0);
}

void FlvWritter::WriteAVCSequenceHeaderTag(
    const char* spsBuf, int spsSize,
    const char* ppsBuf, int ppsSize)
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
    pbuf = UI08ToBytes(pbuf, spsBuf[1]);      // AVCProfileIndication
    pbuf = UI08ToBytes(pbuf, spsBuf[2]);      // profile_compatibility
    pbuf = UI08ToBytes(pbuf, spsBuf[3]);      // AVCLevelIndication
    pbuf = UI08ToBytes(pbuf, 0xff);         // 6 bits reserved (111111) + 2 bits nal size length - 1
    pbuf = UI08ToBytes(pbuf, 0xe1);         // 3 bits reserved (111) + 5 bits number of sps (00001)
    pbuf = UI16ToBytes(pbuf, (unsigned short)spsSize);    // sps
    memcpy(pbuf, spsBuf, spsSize);
    pbuf += spsSize;
    pbuf = UI08ToBytes(pbuf, 1);            // number of pps
    pbuf = UI16ToBytes(pbuf, (unsigned short)ppsSize);    // pps
    memcpy(pbuf, ppsBuf, ppsSize);
    pbuf += ppsSize;

    WriteVideoTag(avc_seq_buf, (int)(pbuf - avc_seq_buf), 0);
}

void FlvWritter::WriteAACData(const char* dataBuf, 
    int dataBufLen, long long timestamp)
{
    timestamp -= delayed_pts_;

    if (time_begin_ == -1) time_begin_ = timestamp;

    if (timestamp < time_begin_)
    {
        time_begin_ = 0;
    }

    unsigned int audio_pts = static_cast<unsigned int>(timestamp-time_begin_);

    audio_pts += opening_pts_;

    last_audio_pts_ = audio_pts;

    WriteAACDataTag((char*)dataBuf, dataBufLen, audio_pts);
}

void FlvWritter::WriteAVCData(const char* dataBuf, 
    int dataBufLen, long long timestamp, int isKeyframe)
{
    timestamp -= delayed_pts_;

    if (time_begin_ == -1) time_begin_ = timestamp;

    if (timestamp < time_begin_)
    {
        time_begin_ = 0;
    }

    unsigned int video_pts = static_cast<unsigned int>(timestamp-time_begin_);

    video_pts += opening_pts_;

    last_video_pts_ = video_pts;

    WriteAVCDataTag(dataBuf, dataBufLen, video_pts, isKeyframe);
}

void FlvWritter::WriteAudioDataTag(const char* dataBuf, 
    int dataBufLen, unsigned int timestamp)
{
    timestamp -= delayed_pts_;

    if (time_begin_ == -1) time_begin_ = timestamp;

    if (timestamp < time_begin_)
    {
        time_begin_ = 0;
    }

    unsigned int audio_pts = static_cast<unsigned int>(timestamp-time_begin_);

    audio_pts += opening_pts_;

    last_audio_pts_ = audio_pts;

    WriteAudioTag((char*)dataBuf, dataBufLen, audio_pts);
}

void FlvWritter::WriteVideoDataTag(const char* dataBuf, 
    int dataBufLen, unsigned int timestamp)
{
    timestamp -= delayed_pts_;

    if (time_begin_ == -1) time_begin_ = timestamp;

    if (timestamp < time_begin_)
    {
        time_begin_ = 0;
    }

    unsigned int video_pts = static_cast<unsigned int>(timestamp-time_begin_);

    video_pts += opening_pts_;

    last_video_pts_ = video_pts;

    WriteVideoTag((char*)dataBuf, dataBufLen, video_pts);
}

void FlvWritter::WriteAACDataTag(const char* dataBuf, int dataBufLen,
    long long framePts)
{
    int need_buf_size = dataBufLen + 2;
    if (need_buf_size > audio_mem_buf_size_)
    {
        if (audio_mem_buf_)
            delete[] audio_mem_buf_;
        audio_mem_buf_ = new char[need_buf_size];
        audio_mem_buf_size_ = need_buf_size;
    }

    char* buf = audio_mem_buf_;
    char* pbuf = buf;

    unsigned char flag = 0;
    flag = (10 << 4) |  // soundformat "10 == AAC"
        (3 << 2) |      // soundrate   "3  == 44-kHZ"
        (1 << 1) |      // soundsize   "1  == 16bit"
        1;              // soundtype   "1  == Stereo"

    pbuf = UI08ToBytes(pbuf, flag);

    pbuf = UI08ToBytes(pbuf, 1);    // aac packet type (1, raw)

    memcpy(pbuf, dataBuf, dataBufLen);
    pbuf += dataBufLen;

    WriteAudioTag(buf, (int)(pbuf - buf), framePts);
}

void FlvWritter::WriteAVCDataTag(const char* dataBuf, int dataBufLen,
    long long framePts, int isKeyframe)
{
    int need_buf_size = dataBufLen + 5;
    if (need_buf_size > video_mem_buf_size_)
    {
        if (video_mem_buf_)
            delete[] video_mem_buf_;
        video_mem_buf_ = new char[need_buf_size];
        video_mem_buf_size_ = need_buf_size;
    }

    char* buf = video_mem_buf_;
    char* pbuf = buf;

    unsigned char flag = 0;
    if (isKeyframe)
        flag = 0x17;
    else
        flag = 0x27;

    pbuf = UI08ToBytes(pbuf, flag);

    pbuf = UI08ToBytes(pbuf, 1);    // avc packet type (0, nalu)
    pbuf = UI24ToBytes(pbuf, 0);    // composition time

    memcpy(pbuf, dataBuf, dataBufLen);
    pbuf += dataBufLen;

    WriteVideoTag(buf, (int)(pbuf - buf), framePts);
}

void FlvWritter::WriteAudioTag(char* buf, 
    int bufLen, unsigned int timestamp)
{
    char prev_size[4];

    FlvTag flvtag;
    memset(&flvtag, 0, sizeof(FlvTag));

    flvtag.type = FLV_TAG_TYPE_AUDIO;
    UI24ToBytes((char*)flvtag.datasize, bufLen);
    flvtag.timestamp_ex = (timestamp >> 24) & 0xff;
    flvtag.timestamp[0] = (timestamp >> 16) & 0xff;
    flvtag.timestamp[1] = (timestamp >> 8) & 0xff;
    flvtag.timestamp[2] = (timestamp) & 0xff;

    fwrite(&flvtag, sizeof(FlvTag), 1, file_handle_);
    fwrite(buf, 1, bufLen, file_handle_);

    UI32ToBytes(prev_size, bufLen+sizeof(FlvTag));
    fwrite(prev_size, 4, 1, file_handle_);
}

void FlvWritter::WriteVideoTag(char* buf, 
    int bufLen, unsigned int timestamp)
{
    char prev_size[4];

    FlvTag flvtag;
    memset(&flvtag, 0, sizeof(FlvTag));

    flvtag.type = FLV_TAG_TYPE_VIDEO;
    UI24ToBytes((char*)flvtag.datasize, bufLen);
    flvtag.timestamp_ex = (timestamp >> 24) & 0xff;
    flvtag.timestamp[0] = (timestamp >> 16) & 0xff;
    flvtag.timestamp[1] = (timestamp >> 8) & 0xff;
    flvtag.timestamp[2] = (timestamp) & 0xff;

    fwrite(&flvtag, sizeof(FlvTag), 1, file_handle_);
    fwrite(buf, 1, bufLen, file_handle_);

    UI32ToBytes(prev_size, bufLen+sizeof(FlvTag));
    fwrite(prev_size, 4, 1, file_handle_);

    //fflush(file_handle_);
}

// 添加音频/视频片头数据
void FlvWritter::WriteAudioOpeningData(const char* dataBuf, int bufLen,
    long long pts)
{
    if (time_begin_ == -1) time_begin_ = pts;

    //WriteAudioTag((char*)dataBuf, bufLen, pts);
    WriteAACDataTag(dataBuf, bufLen, pts);
}

void FlvWritter::WriteVideoOpeningData(const char* dataBuf, int bufLen,
    long long pts, bool isKey)
{
    if (time_begin_ == -1) time_begin_ = pts;

    //WriteVideoTag((char*)dataBuf, bufLen, pts);
    WriteAVCDataTag(dataBuf, bufLen, pts, isKey);
}

// 添加音频/视频片尾数据
void FlvWritter::WriteAudioEndingData(const char* dataBuf, int bufLen,
    long long pts)
{
    //WriteAudioTag((char*)dataBuf, bufLen, last_audio_pts_ + pts);
    WriteAACDataTag((char*)dataBuf, bufLen, last_audio_pts_ + pts);
}

void FlvWritter::WriteVideoEndingData(const char* dataBuf, int bufLen,
    long long pts, bool isKey)
{
    //WriteVideoTag((char*)dataBuf, bufLen, last_video_pts_ + pts);
    WriteAVCDataTag((char*)dataBuf, bufLen, last_video_pts_ + pts, isKey);
}

void FlvWritter::SetOpeningDuration(long long duration)
{
    opening_pts_ = duration;
}

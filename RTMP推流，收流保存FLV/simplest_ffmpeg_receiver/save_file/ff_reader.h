/*******************************************************************************
 * ff_reader.h
 * Copyright: (c) 2014 Haibin Du(haibindev.cnblogs.com). All rights reserved.
 * -----------------------------------------------------------------------------
 *
 *
 *
 * -----------------------------------------------------------------------------
 * 2015-9-7 19:36 - Created (Haibin Du)
 ******************************************************************************/

#ifndef _HDEV_FF_READER_H_
#define _HDEV_FF_READER_H_

//#include "base/base.h"

#include <string>

extern "C"
{
#include "libavformat/avformat.h"
}

class FFReader
{
public:
    explicit FFReader();

    virtual ~FFReader();

    /***
     * �������ļ�
     * @param readUrl: �ļ���
     * @returns:
     */
    bool Open(const std::string& readFile);

    void Close();

    /***
     * ��ȡһ֡����
     * @param reaSize: �����ȡ֡�Ĵ�С
     * @param readType: �����ȡ֡�����ͣ�-1��ȡ����0-�����ݣ�1-��Ƶ��2-��Ƶ
     * @returns:
     */
    char* ReadFrame(int* readType, int* readSize, long long* frameTime,
        bool* isKeyframe = NULL);

	char* GetFrame(int* readType,AVFormatContext* ff_fmt_ctx_, AVPacket reading_pakt_, int* readSize, long long* frameTime,bool *iskeyframe,int videoindex);

    void FreeFrame();

    int Width() { return width_; }

    int Height() { return height_; }

    long long AudioDuration() { return audio_duration_; }

    long long VideoDuration() { return video_duration_; }

    AVCodecID AudioCodecId() { return audio_id_; }

    AVCodecID VideoCodecId() { return video_id_; }

    bool IsPicture() { return is_picture_; }

private:
    // ����
    bool FFOpenStream();

    // ��ȡ���������
    bool FFFindCodecInfo();

public:
	
private:
    std::string filename_;

	AVFormatContext* ff_fmt_ctx_;
    int audio_stream_index_;
    int video_stream_index_;
    AVCodecID audio_id_;
    AVCodecID video_id_;

    AVBitStreamFilterContext* bsfc_;
    bool is_picture_;

    AVPacket reading_pakt_;

    int audio_timebase_;
    int video_timebase_;
    long long a_timestamp_;
    long long v_timestamp_;

    int width_;
    int height_;
    int sample_rate_;
    int channel_count_;
    long long audio_duration_;
    long long video_duration_;

    bool is_has_audio_;
    bool is_has_video_;
};

#endif // _HDEV_FF_READER_H_

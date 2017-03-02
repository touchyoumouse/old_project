//
// FlvWritter
// ����FLV��ʽ�ļ�
//

#ifndef _FLV_WRITTER_H_
#define _FLV_WRITTER_H_

#include "base/base.h"

#include <stdio.h>
#include <string>

class FlvWritter
{
public:
    FlvWritter();

    ~FlvWritter();

    bool Open(const char* filename);

    void Close();

    void SetPause(bool isPause, long long timestamp);

    std::string Filename() { return filename_; }

    // д��FLV�ļ�ͷ
    void WriteFlvHeader(bool isHaveAudio, bool isHaveVideo);

    // д��FLV�ļ�ͷ(ֱ�Ӱ�headerBufд���ļ�)
    void WriteFlvHeader(const char* headerBuf, int bufLen);

    // д��flv aac sequenceHeader tag
    void WriteAACSequenceHeaderTag(int sampleRate, int channel);

    // д��flv avc sequenceHeader tag
    void WriteAVCSequenceHeaderTag(
        const char* spsBuf, int spsSize,
        const char* ppsBuf, int ppsSize);

    // д��flv aac tag
    void WriteAACData(const char* dataBuf, 
        int dataBufLen, long long timestamp);

    // д��flv avc tag
    void WriteAVCData(const char* dataBuf, 
        int dataBufLen, long long timestamp, int isKeyframe);

    // д��flvһ����Ƶtag���ݣ�һ���Ǵ�FLV�ļ��ж�ȡ����tag���ݣ�
    void WriteAudioDataTag(const char* dataBuf, 
        int dataBufLen, unsigned int timestamp);

    // д��flvһ����Ƶtag���ݣ�һ���Ǵ�FLV�ļ��ж�ȡ����tag���ݣ�
    void WriteVideoDataTag(const char* dataBuf, 
        int dataBufLen, unsigned int timestamp);

    // �����Ƶ/��ƵƬͷ����
    void WriteAudioOpeningData(const char* dataBuf, int bufLen, long long pts);
    void WriteVideoOpeningData(const char* dataBuf, int bufLen, long long pts, bool isKey);

    // �����Ƶ/��ƵƬβ����
    void WriteAudioEndingData(const char* dataBuf, int bufLen, long long pts);
    void WriteVideoEndingData(const char* dataBuf, int bufLen, long long pts, bool isKey);

    // ����Ƭͷʱ��������
    void SetOpeningDuration(long long duration);

private:
    void WriteAudioTag(char* buf, 
        int bufLen, unsigned int timestamp);

    void WriteVideoTag(char* buf, 
        int bufLen, unsigned int timestamp);

    void WriteAACDataTag(const char* dataBuf, int dataBufLen,
        long long framePts);

    void WriteAVCDataTag(const char* dataBuf, int dataBufLen,
        long long framePts, int isKeyframe);

private:
    FILE* file_handle_;
    long long time_begin_;
    std::string filename_;

    char* audio_mem_buf_;
    int audio_mem_buf_size_;
    char* video_mem_buf_;
    int video_mem_buf_size_;

    long long last_pause_pts_;
    long long delayed_pts_;

    // Ƭͷʱ��
    long long opening_pts_;

    long long last_audio_pts_;
    long long last_video_pts_;
};

#endif // _FLV_WRITTER_H_

//
// FlvWritter
// 生成FLV格式文件
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

    // 写入FLV文件头
    void WriteFlvHeader(bool isHaveAudio, bool isHaveVideo);

    // 写入FLV文件头(直接吧headerBuf写入文件)
    void WriteFlvHeader(const char* headerBuf, int bufLen);

    // 写入flv aac sequenceHeader tag
    void WriteAACSequenceHeaderTag(int sampleRate, int channel);

    // 写入flv avc sequenceHeader tag
    void WriteAVCSequenceHeaderTag(
        const char* spsBuf, int spsSize,
        const char* ppsBuf, int ppsSize);

    // 写入flv aac tag
    void WriteAACData(const char* dataBuf, 
        int dataBufLen, long long timestamp);

    // 写入flv avc tag
    void WriteAVCData(const char* dataBuf, 
        int dataBufLen, long long timestamp, int isKeyframe);

    // 写入flv一个音频tag数据（一般是从FLV文件中读取到的tag数据）
    void WriteAudioDataTag(const char* dataBuf, 
        int dataBufLen, unsigned int timestamp);

    // 写入flv一个视频tag数据（一般是从FLV文件中读取到的tag数据）
    void WriteVideoDataTag(const char* dataBuf, 
        int dataBufLen, unsigned int timestamp);

    // 添加音频/视频片头数据
    void WriteAudioOpeningData(const char* dataBuf, int bufLen, long long pts);
    void WriteVideoOpeningData(const char* dataBuf, int bufLen, long long pts, bool isKey);

    // 添加音频/视频片尾数据
    void WriteAudioEndingData(const char* dataBuf, int bufLen, long long pts);
    void WriteVideoEndingData(const char* dataBuf, int bufLen, long long pts, bool isKey);

    // 设置片头时长，毫秒
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

    // 片头时长
    long long opening_pts_;

    long long last_audio_pts_;
    long long last_video_pts_;
};

#endif // _FLV_WRITTER_H_

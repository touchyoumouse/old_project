#ifndef _FF_WRITTER_H_
#define _FF_WRITTER_H_

#include <string>

extern "C"
{
#include "libavformat/avformat.h"
#include "libavutil/time.h"
}

#include "base/TimeCounter.h"

class FFWritter
{
public:
    FFWritter(const std::string& fileName, const std::string& fmtName);

    ~FFWritter();

    bool Open(bool isHasVideo, AVCodecID videoId,
        int width, int height, int fps, int bitrate,
        bool isHasAudio, AVCodecID audioId,
        int sampleRate, int channelCount, int audioBitrate = 64000);

    void Close();

    void SetPause(bool isPause);

    // �����Ƶ��������
    void WriteAudioEncodedData(const char* dataBuf, int bufLen,
        long long delayMillsecs = 0);

    // �����Ƶ��������
    void WriteVideoEncodedData(const char* dataBuf, int bufLen, bool isKeyframe,
        long long shouldTS);

    // ����aac������Ϣ���ݣ������Ҫ��
    void SetAudioExtraData(const char* extraData, int extraSize);

    // ����avc������Ϣ���ݣ������Ҫ��
    void SetVideoExtraData(const char* extraData, int extraSize);

    // �����Ƶ/��ƵƬͷ����
    void WriteAudioOpeningData(const char* dataBuf, int bufLen, long long pts);
    void WriteVideoOpeningData(const char* dataBuf, int bufLen, long long pts);

    // �����Ƶ/��ƵƬβ����
    void WriteAudioEndingData(const char* dataBuf, int bufLen, long long pts);
    void WriteVideoEndingData(const char* dataBuf, int bufLen, long long pts);

    // ����Ƭͷʱ��������
    void SetOpeningDuration(long long audioDuration, long long videoDuration);

    unsigned int FileSize();

private:
    void AddAudioStream(int sampleRate, int channelCount, int bitrate = 64000);

    void AddVideoStream(int width, int height, int fps, int bitrate);

    void OpenAudio();

    void OpenVideo();

    void WriteAudioFrame(const char* dataBuf, int bufLen,
        long long framePts);

    void WriteVideoFrame(const char* dataBuf, int bufLen,
        bool isKeyframe, long long framePts);

private:
    std::string filename_;
    std::string fmt_name_;
    AVFormatContext* fmt_ctx_;

    AVCodecID video_codec_id_;
    AVCodecID audio_codec_id_;

    AVOutputFormat* out_fmt_;
    AVStream* audio_stream_;
    AVStream* video_stream_;

    double audio_timebase_;
    double video_timebase_;

    long long audio_pts_;
    double audio_duration_;
    long long audio_frame_count_;
    long long video_pts_;
    double video_duration_;

    long long last_pause_pts_;
    long long delayed_pts_;

    // Ƭͷʱ��
    long long opening_pts_;
    long long opening_audio_pts_;

    long long last_video_pts_;
    TimeCounter tc_;
    int audio_diff_millsec_;

    long long audio_total_size_;
};

#endif // _FF_WRITTER_H_

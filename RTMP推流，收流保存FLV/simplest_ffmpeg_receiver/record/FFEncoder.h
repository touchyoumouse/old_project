#ifndef _FF_ENCODER_H_
#define _FF_ENCODER_H_

#include <string>

struct AVCodec;
struct AVCodecContext;
class FFEncoder
{
public:
    FFEncoder();

    virtual ~FFEncoder();

protected:
    AVCodec* ff_codec_;
    AVCodecContext* ff_codec_ctx_;
};

//////////////////////////////////////////////////////////////////////////

struct AVFrame;
struct SwsContext;
class FFVideoEncoder : public FFEncoder
{
public:
    explicit FFVideoEncoder(int videoCodecId);

    ~FFVideoEncoder();

    /***
     * ����
     * @param srcBuf: ����RGB32����
     * @param srcLen: �������ݴ�С
     * @param outBuf: ����������ݵ�ַ
     * @param outLen: �����С
     * @param isKeyframe: �Ƿ��ǹؼ�֡
     * @returns: ����0����������ɹ�һ֡����
     */
    virtual int Encode(const char* srcBuf, int srcLen, 
        char* outBuf, int& outLen, bool& isKeyframe);
    
    /***
     * ����
     * @param srcBuf: ����YUV420P����
     * @param srcLen: �������ݴ�С
     * @param outBuf: ����������ݵ�ַ
     * @param outLen: �����С
     * @param isKeyframe: �Ƿ��ǹؼ�֡
     * @returns: ����0����������ɹ�һ֡����
     */
    int EncodeYuv(const char* srcBuf, int srcLen, 
        char* outBuf, int& outLen, bool& isKeyframe);

    /***
     * ��ʼ��������
     * @param width: ���
     * @param height: �߶�
     * @param bitrate: ��Ƶ����
     * @param fps: ��Ƶ֡��
     * @returns:
     */
    void Init(int width, int height, int fps, int bitRate);

private:
    void AllocFrameBuf();

private:
    AVFrame* ff_yuv_frame_;
    unsigned char* ff_yuv_buf_;
    int ff_yuv_buf_size_;

    AVFrame* ff_rgb_frame_;
    unsigned char* ff_rgb_buf_;
    int ff_rgb_buf_size_;

    SwsContext* ff_sws_ctx_;
};

//////////////////////////////////////////////////////////////////////////

class FFAudioEncoder : public FFEncoder
{
public:
    explicit FFAudioEncoder(int audioCodecId);

    ~FFAudioEncoder();

    virtual int Encode(unsigned char* buf, int bufLen, 
        unsigned char* outBuf, int& outLen);

    void Init(int sampleRate, int channels, int bitRate);

    int BufSize();

    void SetbufSize(int bufSize) { bufsize_ = bufSize; }

private:
    int bufsize_;

    AVFrame* input_frame_;
};

#endif // _FF_ENCODER_H_

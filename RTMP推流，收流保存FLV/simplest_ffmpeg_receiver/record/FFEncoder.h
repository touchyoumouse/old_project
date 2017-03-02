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
     * 编码
     * @param srcBuf: 输入RGB32数据
     * @param srcLen: 输入数据大小
     * @param outBuf: 输出编码数据地址
     * @param outLen: 输出大小
     * @param isKeyframe: 是否是关键帧
     * @returns: 大于0，表明编码成功一帧数据
     */
    virtual int Encode(const char* srcBuf, int srcLen, 
        char* outBuf, int& outLen, bool& isKeyframe);
    
    /***
     * 编码
     * @param srcBuf: 输入YUV420P数据
     * @param srcLen: 输入数据大小
     * @param outBuf: 输出编码数据地址
     * @param outLen: 输出大小
     * @param isKeyframe: 是否是关键帧
     * @returns: 大于0，表明编码成功一帧数据
     */
    int EncodeYuv(const char* srcBuf, int srcLen, 
        char* outBuf, int& outLen, bool& isKeyframe);

    /***
     * 初始化编码器
     * @param width: 宽度
     * @param height: 高度
     * @param bitrate: 视频码率
     * @param fps: 视频帧率
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

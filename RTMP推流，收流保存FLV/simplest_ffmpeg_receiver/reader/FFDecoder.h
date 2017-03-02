#ifndef _FF_DECODER_H_
#define _FF_DECODER_H_

#include "../base/base.h"

struct AVCodec;
struct AVCodecContext;
class FFDecoder
{
public:
    FFDecoder(AVCodecContext* codecContex);

    virtual ~FFDecoder();

    virtual int Decode(unsigned char* buf, unsigned int bufLen, 
        unsigned char* outBuf, int& outLen) = 0;

protected:
    AVCodec* ff_codec_;
    AVCodecContext* ff_codec_ctx_;
    bool is_ctx_need_free_;
};

//////////////////////////////////////////////////////////////////////////
// 视频解码器

struct AVFrame;
struct AVPacket;
struct SwsContext;
class FFVideoDecoder : public FFDecoder
{
public:
    /***
     * 从ffmpeg里的参数构造解码器
     * @param codecContex: ffmpeg打开流时获取到的结构
     * @returns:
     */
    FFVideoDecoder(AVCodecContext* codecContex);

    /***
     * 构造函数
     * @param videoCodecId: 视频编码ID
     * @returns:
     */
    explicit FFVideoDecoder(int videoCodecId);

    ~FFVideoDecoder();

    /***
     * 解码，将输入编码数据，还原成RGB原始画面数据
     * @param buf: 输入数据，h264编码数据
     * @param bufLen: 输入数据长度
     * @param outBuf: 输出内存地址
     * @param outLen: 输出内存大小
     * @returns:
     */
    virtual int Decode(unsigned char* buf, unsigned int bufLen, 
        unsigned char* outBuf, int& outLen);

    /***
     * 解码，将输入编码数据，还原成RGB原始画面数据
     * @param avPacket: ffmpeg读取到的数据帧结构体
     * @param outBuf: 输出内存地址
     * @param outLen: 输出内存大小
     * @returns:
     */
    int DecodeAVPacket(AVPacket* avPacket, 
        unsigned char* outBuf, int& outLen);

    /***
     * 初始化解码器
     * @param width: 源画面宽
     * @param height: 源画面高
     * @param inPixFmt: 源数据编码前的YUV格式
     * @param outWidth: 输出画面宽(只有调用Decode或DecodeAVPacket才有效)
     * @param outHeight: 输出画面高
     * @returns:
     */
    void Init(int width, int height, int inPixFmt,
        int outWidth, int outHeight);

    void UpdateOutWH(int outWidth, int outHeight);

    int OutputBufSize() { return ff_out_buf_size_; }

    void SetDeinterlace(bool doDeinterlace) { do_deinterlace_ = doDeinterlace; }

    bool GetDeinterlace() { return do_deinterlace_; }

    /***
     * 将输入编码数据，解码为AVFrame但不返回，之后获取
     * @param buf: 输入数据，h264编码数据
     * @param bufLen: 输入数据长度
     * @returns: 0为没有解码出画面，>0则表示输出画面大小
     */
    int DecodeToFrame(unsigned char* buf, unsigned int bufLen);

    /***
     * 将输入编码数据，解码为AVFrame但不返回，之后获取
     * @param avPacket: ffmpeg读取到的数据帧结构体
     * @returns: 0为没有解码出画面，>0则表示输出画面大小
     */
    int DecodeAVPacketToFrame(AVPacket* avPacket);

    /***
     * 获取上面两个函数解码出来的AVFrame
     */
    AVFrame* GetDecodedFrame() { return ff_decoded_frame_; }

private:
    void AllocFrameBuf();

    void FreeFrameBuf();

private:
    AVFrame* ff_decoded_frame_;
    unsigned char* ff_decoded_buf_;
    int ff_decoded_buf_size_;

    int fps_;
    bool do_deinterlace_;
    AVFrame* ff_deinterlace_frame_;
    unsigned char* ff_deinterlace_buf_;

    AVFrame* ff_out_frame_;
    int ff_out_buf_size_;

    SwsContext* ff_sws_ctx_;
    int out_width_;
    int out_height_;
};

//////////////////////////////////////////////////////////////////////////

class FFAudioDecoder : public FFDecoder
{
public:
    FFAudioDecoder(AVCodecContext* codecContex);

    explicit FFAudioDecoder(int audioCodecId);

    ~FFAudioDecoder();

    virtual int Decode(unsigned char* buf, unsigned int bufLen, 
        unsigned char* outBuf, int& outLen);

    void Init(int sampleRate, int channels);

    int BufSize();

    int SampleFmt() { return sample_fmt_; }

    int SampleRate() { return sample_rate_; };

private:
    int sample_fmt_;
    int sample_rate_;
    AVFrame* pcm_frame_;
};

#endif // _FF_DECODER_H_

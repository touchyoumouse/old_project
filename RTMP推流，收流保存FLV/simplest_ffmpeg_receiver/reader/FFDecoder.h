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
// ��Ƶ������

struct AVFrame;
struct AVPacket;
struct SwsContext;
class FFVideoDecoder : public FFDecoder
{
public:
    /***
     * ��ffmpeg��Ĳ������������
     * @param codecContex: ffmpeg����ʱ��ȡ���Ľṹ
     * @returns:
     */
    FFVideoDecoder(AVCodecContext* codecContex);

    /***
     * ���캯��
     * @param videoCodecId: ��Ƶ����ID
     * @returns:
     */
    explicit FFVideoDecoder(int videoCodecId);

    ~FFVideoDecoder();

    /***
     * ���룬������������ݣ���ԭ��RGBԭʼ��������
     * @param buf: �������ݣ�h264��������
     * @param bufLen: �������ݳ���
     * @param outBuf: ����ڴ��ַ
     * @param outLen: ����ڴ��С
     * @returns:
     */
    virtual int Decode(unsigned char* buf, unsigned int bufLen, 
        unsigned char* outBuf, int& outLen);

    /***
     * ���룬������������ݣ���ԭ��RGBԭʼ��������
     * @param avPacket: ffmpeg��ȡ��������֡�ṹ��
     * @param outBuf: ����ڴ��ַ
     * @param outLen: ����ڴ��С
     * @returns:
     */
    int DecodeAVPacket(AVPacket* avPacket, 
        unsigned char* outBuf, int& outLen);

    /***
     * ��ʼ��������
     * @param width: Դ�����
     * @param height: Դ�����
     * @param inPixFmt: Դ���ݱ���ǰ��YUV��ʽ
     * @param outWidth: ��������(ֻ�е���Decode��DecodeAVPacket����Ч)
     * @param outHeight: ��������
     * @returns:
     */
    void Init(int width, int height, int inPixFmt,
        int outWidth, int outHeight);

    void UpdateOutWH(int outWidth, int outHeight);

    int OutputBufSize() { return ff_out_buf_size_; }

    void SetDeinterlace(bool doDeinterlace) { do_deinterlace_ = doDeinterlace; }

    bool GetDeinterlace() { return do_deinterlace_; }

    /***
     * ������������ݣ�����ΪAVFrame�������أ�֮���ȡ
     * @param buf: �������ݣ�h264��������
     * @param bufLen: �������ݳ���
     * @returns: 0Ϊû�н�������棬>0���ʾ��������С
     */
    int DecodeToFrame(unsigned char* buf, unsigned int bufLen);

    /***
     * ������������ݣ�����ΪAVFrame�������أ�֮���ȡ
     * @param avPacket: ffmpeg��ȡ��������֡�ṹ��
     * @returns: 0Ϊû�н�������棬>0���ʾ��������С
     */
    int DecodeAVPacketToFrame(AVPacket* avPacket);

    /***
     * ��ȡ���������������������AVFrame
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

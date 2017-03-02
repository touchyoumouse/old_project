/*******************************************************************************
 * source_reader.cpp
 * Copyright: (c) 2014 Haibin Du(haibindev.cnblogs.com). All rights reserved.
 * -----------------------------------------------------------------------------
 *
 * -----------------------------------------------------------------------------
 * 2015-6-23 10:10 - Created (Haibin Du)
 ******************************************************************************/

#include "source_reader.h"

#include <sstream>

#include "../base/h264_frame_parser.h"
#include "../base/simple_logger.h"
#include "../record/recoder.h"

#include "FFCodec.h"
#include "FAACDecoder.h"
#include "FFDecoder.h"

#include "service/service_buffer.h"

#define IGNORE_BEGIN_DATA_TIME 500

SourceReader::SourceReader(int sourceId, int sourceType, Observer* ob,
    const ServiceConfig& serviceCfg, const SourceConfig& srcCfg)
{
    source_id_ = sourceId;
    source_type_ = sourceType;
    observer_ = ob;

    service_cfg_ = serviceCfg;
    src_cfg_ = srcCfg;

    src_samplerate_ = 0;
    src_channel_ = 0;

    decode_sample_rate_ = 0;
    decode_channel_ = 0;
    aac_decoder_ = NULL;
    unit_helper_ = NULL;
    ff_resampler_ = NULL;
    sample_pcmbuf_ = NULL;
    sample_pcmsize_ = 0;
    resample_pcmbuf_ = NULL;
    resample_max_sam_nb_ = 0;

    h264_decoder_ = NULL;
    src_width_ = 0;
    src_height_ = 0;
    extra_data_ = NULL;
    extra_size_ = 0;
    has_extra_used_ = false;

    rtmp_reader_ = NULL;
    rtsp_reader_ = NULL;

    audio_queue_ = NULL;
    video_queue_ = NULL;

    audio_last_timestamp_ = -1;
    video_last_timestamp_ = -1;

    is_recording_ = false;
    recorder_ = NULL;

    is_failed_ = false;

    is_audio_codec_init_ = false;
    is_video_codec_init_ = false;

    //swav_out_ = NULL;
}

SourceReader::~SourceReader()
{

}

void SourceReader::Start(const std::string& requestUrl)
{
    SIMPLE_LOG("[%d] %s\n", source_id_, requestUrl.c_str());

    source_url_ = requestUrl;

    total_timer_.Reset();

    switch (source_type_)
    {
    case SOURCE_TYPE_RTSP:
        {
            rtsp_reader_ = new FFUrlReader(this);
            rtsp_reader_->Start(requestUrl);

            SIMPLE_LOG("[%d] pointer: %p\n", source_id_, rtsp_reader_);
        }
        break;
    case SOURCE_TYPE_RTMP:
        {
            rtmp_reader_ = new RtmpReader(false);
            rtmp_reader_->SetObserver(this);
            rtmp_reader_->Start(requestUrl.c_str());
        }
        break;
    }

    ThreadStart();
}

void SourceReader::Stop()
{
    if (IsThreadStop()) return;

    SIMPLE_LOG("[%d] beg join\n", source_id_);

    ThreadStop();
    ThreadJoin();

    SIMPLE_LOG("[%d] try stop\n", source_id_);

    base::AutoLock al(write_mtx_);

    switch (source_type_)
    {
    case SOURCE_TYPE_RTSP:
        {
            rtsp_reader_->Stop();
            delete rtsp_reader_;
            rtsp_reader_ = NULL;
        }
        break;
    case SOURCE_TYPE_RTMP:
        {
            rtmp_reader_->Stop();
            delete rtmp_reader_;
            rtmp_reader_ = NULL;
        }
        break;
    }

    SIMPLE_LOG("[%d] try delete decoder\n", source_id_);

    // reader thread stopped, delete decoder safely

    if (sample_pcmbuf_)
    {
        av_freep(&sample_pcmbuf_[0]);
    }

    if (resample_pcmbuf_)
    {
        av_free(resample_pcmbuf_[0]);
    }

    if (ff_resampler_)
    {
        swr_free(&ff_resampler_);
        ff_resampler_ = NULL;
    }

    if (unit_helper_)
    {
        delete unit_helper_;
    }

    if (aac_decoder_)
    {
        delete aac_decoder_;
        aac_decoder_ = NULL;
    }

    if (h264_decoder_)
    {
        delete h264_decoder_;
        h264_decoder_ = NULL;
    }

    if (recorder_)
    {
        base::AutoLock al(write_mtx_);

        recorder_->Stop();
        delete recorder_;
        recorder_ = NULL;
    }

    if (audio_queue_)
    {
        audio_queue_->Stop();
        delete audio_queue_;
        audio_queue_ = NULL;
    }
    if (video_queue_)
    {
        video_queue_->Stop();
        delete video_queue_;
        video_queue_ = NULL;
    }

    SIMPLE_LOG("[%d] end\n", source_id_);

    //delete wav_out_;
}

void SourceReader::StartRecord()
{
    std::vector<std::string> exts;
    exts.push_back(".flv");
    std::string filename = ServiceConfig::NextValidFilename(src_cfg_.record_dir_,
        src_cfg_.record_basename_, exts);

    base::AutoLock al(write_mtx_);

    if (src_width_ > 0/* && src_samplerate_ > 0*/)
    {
        recorder_ = new Recorder(Recorder::SAVE_FILE_FLV, filename);

        recorder_->Start(true, src_samplerate_, src_channel_,
            src_cfg_.need_audio_, src_width_, src_height_, 0, 0);
    }

    is_recording_ = true;
}

void SourceReader::PauseRecord()
{
     base::AutoLock al(write_mtx_);

     if (recorder_)
     {
         recorder_->Pause();
     }
}

void SourceReader::StopRecord()
{
    base::AutoLock al(write_mtx_);

    is_recording_ = false;

    if (recorder_)
    {
        recorder_->Stop();
        delete recorder_;
        recorder_ = NULL;
    }
}

void SourceReader::UpdateConfig(const ServiceConfig& serviceCfg,
    const SourceConfig& srcCfg)
{
    SIMPLE_LOG("[%d] beg\n", source_id_);

    base::AutoLock al(write_mtx_);

    SIMPLE_LOG("[%d]\n", source_id_);

    if (serviceCfg.RecordWidth() != service_cfg_.RecordWidth() ||
        serviceCfg.RecordHeight() != service_cfg_.RecordHeight())
    {
        SIMPLE_LOG("[%d]\n", source_id_);

        rgbbuf_.resize(serviceCfg.RecordWidth()*serviceCfg.RecordHeight()*4);
        if (h264_decoder_)
        {
            SIMPLE_LOG("[%d]\n", source_id_);

            h264_decoder_->UpdateOutWH(serviceCfg.RecordWidth(),
                serviceCfg.RecordHeight());

            SIMPLE_LOG("[%d]\n", source_id_);
        }
    }

    if (serviceCfg.PubSampleRate() != service_cfg_.PubSampleRate())
    {
        // 重设音频重采样参数

        CreateAudioResample(serviceCfg.PubSampleRate());
    }

    service_cfg_ = serviceCfg;
    src_cfg_ = srcCfg;

    SIMPLE_LOG("[%d] end\n", source_id_);
}

// -------------------------------------------
// base::SimpleThread

void SourceReader::ThreadRun()
{
    TimeCounter check_tc;
    check_tc.Reset();

    while (false == IsThreadStop())
    {
        if (check_tc.Get() >= 3000)
        {
            OnCheckTimer();

            check_tc.Reset();
        }

        MillsecSleep(100);
    }

    SIMPLE_LOG("[%d] thread end\n", source_id_);
}

// --------------------------------------------------------
// FFUrlReader::Observer

void SourceReader::OnFFEvent(int eventId)
{
    // 回调里需要异步一下再处理

    if (eventId == FFUrlReader::FF_URL_OPEN_FAILED)
    {
        is_failed_ = true;
    }
    else if (eventId == FFUrlReader::FF_URL_ABORT)
    {
        is_failed_ = true;
    }

    if (observer_)
    {
        observer_->OnSourceReaderEvent(source_id_, eventId);
    }
}

void SourceReader::OnFFCodecInfo(
    int audioCodec, int sampleRate, int channelCount,
    int videoCodec, int width, int height, int pixFmt,
    const char* extraBuf, int extraSize)
{
    if (IsThreadStop()) return;

    SIMPLE_LOG("[%d] beg\n", source_id_);
    {
        //base::AutoLock al(write_mtx_);
        write_mtx_.Acquire();

        SIMPLE_LOG("[%d]\n", source_id_);

        // 如果解码器已经初始化了，就直接返回
        if (is_audio_codec_init_ || is_video_codec_init_)
        {
            write_mtx_.Release();

            audio_queue_->Clear();
            video_queue_->Clear();

            return;
        }

        is_audio_codec_init_ = true;
        is_video_codec_init_ = true;

        SIMPLE_LOG("[%d]\n", source_id_);

        CreateAudioDecoder(audioCodec, sampleRate, channelCount);

        SIMPLE_LOG("[%d]\n", source_id_);

        CreateVideoDecoder(videoCodec, width, height, pixFmt);

        write_mtx_.Release();

        if (observer_)
        {
            if (sampleRate < 24000)
            {
                sampleRate = sampleRate*2;
            }
            channelCount = 2;

            SIMPLE_LOG("[%d]\n", source_id_);

            if (false == src_cfg_.need_audio_)
            {
                sampleRate = 0;
                channelCount = 0;
            }
            observer_->OnSourceReaderInfo(source_id_,
                sampleRate, channelCount, width, height);
        }
    }

    if (extraBuf && extraSize > 0)
    {
        extra_data_ = new char[extraSize];
        memcpy(extra_data_, extraBuf, extraSize);
        extra_size_ = extraSize;

        has_extra_used_ = false;
    }

    SIMPLE_LOG("[%d] end\n", source_id_);
}

void SourceReader::OnFFAudioBuf(const char* dataBuf, int dataSize,
    long long timestamp, long long duration)
{
    if (IsThreadStop()) return;

    SIMPLE_LOG("[%d] beg, %lld\n", source_id_, timestamp);

    // 判断配置里是否需要音频数据
    if (src_cfg_.need_audio_)
    {
        //DecodeAudio(dataBuf, dataSize, timestamp);
        audio_queue_->Post(dataBuf, dataSize, timestamp);
    }
    
    RecordAudio(dataBuf, dataSize, timestamp);

    SIMPLE_LOG("[%d] end\n", source_id_);
}

FILE* gF264[3];

void SourceReader::OnFFVideoBuf(const char* dataBuf, int dataSize,
    long long timestamp, long long duration)
{
    if (IsThreadStop()) return;

    is_failed_ = false;

    SIMPLE_LOG("[%d] beg, %lld\n", source_id_, timestamp);

    if (0)
    {
        char fnamebuf[20] = {0};
        std::string fname = itoa(source_id_, fnamebuf, 10);
        fname = "test_" + fname + ".264";
        if (gF264[source_id_] == NULL)
        {
            gF264[source_id_] = fopen(fname.c_str(), "wb");
        }
        fwrite(dataBuf, dataSize, 1, gF264[source_id_]);
        fflush(gF264[source_id_]);
    }

    //DecodeVideo(dataBuf, dataSize);
    if (extra_data_ && !has_extra_used_)
    {
        video_queue_->Post(extra_data_, extra_size_, timestamp);
        RecordVideo(extra_data_, extra_size_, timestamp);

        has_extra_used_ = true;
    }

    video_queue_->Post(dataBuf, dataSize, timestamp);

    RecordVideo(dataBuf, dataSize, timestamp);

    SIMPLE_LOG("[%d] end\n", source_id_);
}

void SourceReader::OnFFAVPacket(AVPacket* avPacket,
    long long timestamp, long long duration)
{
    if (IsThreadStop()) return;

    is_failed_ = false;

    if (0)
    {
        if (source_id_ != 0) return;

        char fnamebuf[20] = {0};
        std::string fname = itoa(source_id_, fnamebuf, 10);
        fname = "test_" + fname + ".264";
        if (gF264[source_id_] == NULL)
        {
            gF264[source_id_] = fopen(fname.c_str(), "wb");
        }
        fwrite(avPacket->data, avPacket->size, 1, gF264[source_id_]);
        fflush(gF264[source_id_]);
    }

    SIMPLE_LOG("[%d] beg, %lld\n", source_id_, timestamp);

    if (extra_data_ && !has_extra_used_)
    {
        DecodeVideo(extra_data_, extra_size_, timestamp);
        RecordVideo(extra_data_, extra_size_, timestamp);

        has_extra_used_ = true;
    }

    DecodeAVPacket(avPacket, timestamp);

	//@feng 并没有保存
    RecordVideo((char*)avPacket->data, avPacket->size, timestamp);

    SIMPLE_LOG("[%d] end\n", source_id_);
}

void SourceReader::OnAudioQueueData(const char* dataBuf, int dataSize)
{
    if (IsThreadStop()) return;

    DecodeAudio(dataBuf, dataSize);
}

void SourceReader::OnVideoQueueData(const char* dataBuf, int dataSize,
    long long timestamp)
{
    if (IsThreadStop()) return;

    SIMPLE_LOG("[%d] beg\n", source_id_);

    DecodeVideo(dataBuf, dataSize, timestamp);
}

// --------------------------------------------------------
// RtmpReader::Observer

void SourceReader::OnRtmpErrCode(int errCode)
{
    if (IsThreadStop()) return;

    if (errCode == RtmpReader::kRtmpConnectFailed ||
        errCode == RtmpReader::kRtmpRecvFailed)
    {
        is_failed_ = true;
    }

    if (observer_)
    {
        //observer_->OnSourceReaderEvent(source_id_, errCode);
    }
}

void SourceReader::OnRtmpAudioCodec(int sampleRate, int channelCount)
{
    if (IsThreadStop()) return;

    SIMPLE_LOG("[%d] beg\n", source_id_);

    //base::AutoLock al(write_mtx_);

    write_mtx_.Acquire();

    if (is_audio_codec_init_)
    {
        write_mtx_.Release();

        return;
    }
    is_audio_codec_init_ = true;

    CreateAudioDecoder(AV_CODEC_ID_AAC, sampleRate, channelCount);

    write_mtx_.Release();



    if (is_audio_codec_init_ && is_video_codec_init_ && observer_)
    {
        if (false == src_cfg_.need_audio_)
        {
            sampleRate = 0;
            channelCount = 0;
        }
        else
        {
            if (sampleRate < 24000)
            {
                sampleRate = sampleRate*2;
            }
            channelCount = 2;
        }
        observer_->OnSourceReaderInfo(source_id_, sampleRate, channelCount,
            rtmp_reader_->Width(), rtmp_reader_->Height());
    }

    SIMPLE_LOG("[%d] end\n", source_id_);
}

void SourceReader::OnRtmpVideoCodec(int width, int height)
{
    if (IsThreadStop()) return;

    SIMPLE_LOG("[%d] beg\n", source_id_);

    write_mtx_.Acquire();

    if (is_video_codec_init_)
    {
        write_mtx_.Release();
        return;
    }
    is_video_codec_init_ = true;

    CreateVideoDecoder(AV_CODEC_ID_H264, width, height, AV_PIX_FMT_YUV420P);

    write_mtx_.Release();

    if (false == src_cfg_.need_audio_)
    {
        // 如果配置里不需要音频，则收到视频编码后就可以直接初始化了
        is_audio_codec_init_ = true;
    }

    if (is_audio_codec_init_ && is_video_codec_init_ && observer_)
    {
        int sample_rate = 0;
        int channel = 0;
        if (src_cfg_.need_audio_)
        {
            sample_rate = rtmp_reader_->Samplerate();
            channel = rtmp_reader_->Channel();

            if (sample_rate < 24000)
            {
                sample_rate = sample_rate*2;
            }
            channel = 2;
        }

        observer_->OnSourceReaderInfo(source_id_, 
            sample_rate, channel, width, height);
    }

    SIMPLE_LOG("[%d] end\n", source_id_);
}

void SourceReader::OnRtmpAudioBuf(const char* dataBuf, unsigned int dataLen,
    long long timestamp)
{
    if (IsThreadStop()) return;

    SIMPLE_LOG("[%d] beg\n", source_id_);

    if (src_cfg_.need_audio_)
    {
        DecodeAudio(dataBuf, dataLen);
    }

    RecordAudio(dataBuf, dataLen, timestamp);
}

void SourceReader::OnRtmpVideoBuf(const char* dataBuf, unsigned int dataLen,
    long long timestamp)
{
    if (IsThreadStop()) return;

    is_failed_ = false;

    SIMPLE_LOG("[%d] beg, %.3f\n", source_id_, timestamp);

    DecodeVideo(dataBuf, dataLen, timestamp);

    RecordVideo(dataBuf, dataLen, timestamp);
}

void SourceReader::CreateAudioDecoder(int audioCodec, int sampleRate,
    int channelCount)
{
    src_samplerate_ = sampleRate;
    src_channel_ = channelCount;

    if (audioCodec > 0 && sampleRate > 0)
    {
        aac_decoder_ = new FAACDecoder();
        aac_decoder_->Init(sampleRate, channelCount, 16);

        channelCount = 2;
        pcmbuf_.resize(1024*channelCount*20);

        decode_sample_rate_ = sampleRate;
        if (decode_sample_rate_ < 24000) decode_sample_rate_ = decode_sample_rate_*2;
        decode_channel_ = 2;

        CreateAudioResample(service_cfg_.PubSampleRate());

        audio_queue_ = new AudioBufferingQueue(this, decode_sample_rate_);
        audio_queue_->Start();
    }

    // 检查参数是否获取完全，并判断是否开启录制
    if (is_recording_ && recorder_ == NULL)
    {
        StartRecord();
    }
}

void SourceReader::CreateAudioResample(int toSampleRate)
{
    if (sample_pcmbuf_)
    {
        av_freep(&sample_pcmbuf_[0]);
        sample_pcmbuf_ = NULL;
    }
    sample_pcmsize_ = 0;

    if (resample_pcmbuf_)
    {
        av_free(resample_pcmbuf_[0]);
        resample_pcmbuf_ = NULL;
    }
    resample_max_sam_nb_ = 0;

    if (ff_resampler_)
    {
        swr_free(&ff_resampler_);
        ff_resampler_ = NULL;
    }

    if (unit_helper_)
    {
        delete unit_helper_;
        unit_helper_ = NULL;
    }

    int aac_unit_size = 1024*decode_channel_*2;
    unit_helper_ = new BufUnitHelper(aac_unit_size);

    ff_resampler_ = swr_alloc();
    /* set options */
    av_opt_set_int(ff_resampler_, "in_channel_layout",    AV_CH_LAYOUT_STEREO, 0);
    av_opt_set_int(ff_resampler_, "in_sample_rate",       decode_sample_rate_, 0);
    av_opt_set_sample_fmt(ff_resampler_, "in_sample_fmt", AV_SAMPLE_FMT_S16, 0);

    av_opt_set_int(ff_resampler_, "out_channel_layout",    AV_CH_LAYOUT_STEREO, 0);
    av_opt_set_int(ff_resampler_, "out_sample_rate",       toSampleRate, 0);
    av_opt_set_sample_fmt(ff_resampler_, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);
    int avret = swr_init(ff_resampler_);

//     if (src_cfg_.need_audio_)
//     {
//         wav_out_ = new WavOutFile("test.wav", toSampleRate, 16, 2);
//     }
}

void SourceReader::DecodeAudio(const char* dataBuf, int dataSize)
{
    if (IsThreadStop()) return;

    if (total_timer_.Get() < IGNORE_BEGIN_DATA_TIME)
    {
        SIMPLE_LOG("[%d] ignore begining data\n", source_id_);
        return;
    }

    SIMPLE_LOG("[%d] beg\n", source_id_);

    write_mtx_.Acquire();

    unsigned int outlen = 0;
    aac_decoder_->Decode((unsigned char*)dataBuf, dataSize,
        (unsigned char*)&pcmbuf_[0], outlen);

    SIMPLE_LOG("[%d] size: %d\n", source_id_, outlen);

    if (outlen > 0 && observer_)
    {
        if (decode_sample_rate_ != service_cfg_.PubSampleRate())
        {
            // 需要做重采样
            int dst_bufsize = ResampleAudio(&pcmbuf_[0], outlen);

            //if (wav_out_) wav_out_->write((short*)&pcmbuf_[0], outlen/2);
            //if (wav_out_) wav_out_->write((short*)resample_pcmbuf_[0], dst_bufsize/2);

            if (dst_bufsize > 0)
            {
                ResizeAudioBufUnderMutex((char*)resample_pcmbuf_[0], dst_bufsize);
            }

            write_mtx_.Release();
        }
        else
        {
            // 解码出pcm数据，直接回调
            if (outlen == unit_helper_->UnitSize())
            {
                write_mtx_.Release();

                if (audio_arrive_timer_.LastTimestamp() < 0)
                {
                    audio_arrive_timer_.Reset();
                }

                observer_->OnSourceReaderAudioBuf(source_id_, &pcmbuf_[0], outlen);
            }
            else
            {
                ResizeAudioBufUnderMutex(&pcmbuf_[0], outlen);

                write_mtx_.Release();
            }
        }
    }
    else
    {
        write_mtx_.Release();
    }

    SIMPLE_LOG("[%d] end\n", source_id_);
}

void SourceReader::ResizeAudioBufUnderMutex(char* dataBuf, int dataSize)
{
    unit_helper_->PushBuf(dataBuf, dataSize);

    while (unit_helper_->ReadBuf())
    {
        write_mtx_.Release();

        if (audio_arrive_timer_.LastTimestamp() < 0)
        {
            audio_arrive_timer_.Reset();
        }

        observer_->OnSourceReaderAudioBuf(source_id_, unit_helper_->UnitBuf(),
            unit_helper_->UnitSize());

        write_mtx_.Acquire();
    }
}

int SourceReader::ResampleAudio(const char* dataBuf, int dataSize)
{
    int dst_samrate = service_cfg_.PubSampleRate();
    int dst_channel = 2;

    int src_sam_bytes = av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
    int src_sam_nb = (dataSize / src_sam_bytes) / decode_channel_;
    int src_linesize;
    if (sample_pcmbuf_ == NULL)
    {
        av_samples_alloc_array_and_samples(&sample_pcmbuf_, &src_linesize, decode_channel_,
            src_sam_nb, AV_SAMPLE_FMT_S16, 1);
    }
    memcpy(sample_pcmbuf_[0], dataBuf, dataSize);

    if (resample_max_sam_nb_ == 0)
    {
        int tmp_dst_samnb =
            av_rescale_rnd(src_sam_nb, dst_samrate, decode_sample_rate_, AV_ROUND_UP);
        av_samples_alloc_array_and_samples(&resample_pcmbuf_, &dst_linesize_, dst_channel,
            tmp_dst_samnb, AV_SAMPLE_FMT_S16, 1);
        resample_max_sam_nb_ = tmp_dst_samnb;
    }

    int dst_sam_nb = av_rescale_rnd(swr_get_delay(ff_resampler_, decode_sample_rate_) +src_sam_nb,
        dst_samrate, decode_sample_rate_, AV_ROUND_UP);
    int dst_samp_bytes = av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);

    if (dst_sam_nb > resample_max_sam_nb_)
    {
        av_free(resample_pcmbuf_[0]);
        av_samples_alloc(resample_pcmbuf_, &dst_linesize_, dst_channel,
            dst_sam_nb, AV_SAMPLE_FMT_S16, 1);
        resample_max_sam_nb_ = dst_sam_nb;
    }

    int ret = swr_convert(ff_resampler_, resample_pcmbuf_, dst_sam_nb,
        (const uint8_t**)sample_pcmbuf_, src_sam_nb);
    if (ret >= 0)
    {
        int dst_bufsize = av_samples_get_buffer_size(&dst_linesize_, 
            dst_channel, ret, AV_SAMPLE_FMT_S16, 1);
        //int dst_bufsize = ret * dst_samp_bytes;
        return dst_bufsize;
    }
    return 0;
}

void SourceReader::CreateVideoDecoder(int videoCodec, int width, int height,
    int pixFmt)
{
    src_width_ = width;
    src_height_ = height;

    if (videoCodec > 0 && width > 0)
    {
        h264_decoder_ = new FFVideoDecoder(videoCodec);

        h264_decoder_->Init(width, height, pixFmt,
            service_cfg_.RecordWidth(), service_cfg_.RecordHeight());

        rgbbuf_.resize(service_cfg_.RecordWidth()*service_cfg_.RecordHeight()*4);

        video_queue_ = new VideoBufferingQueue(this);
        video_queue_->Start();
    }

    // 检查参数是否获取完全，并判断是否开启录制
    if (is_recording_ && recorder_ == NULL)
    {
        StartRecord();
    }
}

void SourceReader::DecodeVideo(const char* dataBuf, int dataSize,
    long long timestamp)
{
    if (IsThreadStop()) return;

    SIMPLE_LOG("[%d] beg\n", source_id_);

    if (video_last_timestamp_ < 0)
        video_last_timestamp_ = timestamp;

    //write_mtx_.Acquire();

    int outlen = 0;
    outlen = h264_decoder_->DecodeToFrame((unsigned char*)dataBuf, dataSize);

    //write_mtx_.Release();

    if (total_timer_.Get() < IGNORE_BEGIN_DATA_TIME)
    {
        SIMPLE_LOG("[%d] ignore begining data\n", source_id_);
        return;
    }

    if (video_arrive_timer_.LastTimestamp() < 0)
    {
        video_arrive_timer_.Reset();
    }

    if (outlen > 0 && observer_)
    {
        SIMPLE_LOG("[%d]\n", source_id_);
        // 解码出原始图像，回调
        long long diff_time = timestamp - video_last_timestamp_;
        observer_->OnSourceReaderVideoBuf(source_id_,
            h264_decoder_->GetDecodedFrame(), diff_time);
    }

    SIMPLE_LOG("[%d] end\n", source_id_);
}

void SourceReader::DecodeAVPacket(AVPacket* avPacket, long long timestamp)
{
    if (IsThreadStop()) return;

    SIMPLE_LOG("[%d] beg\n", source_id_);

    if (video_last_timestamp_ < 0)
        video_last_timestamp_ = timestamp;

    //write_mtx_.Acquire();

    int outlen = 0;
    outlen = h264_decoder_->DecodeAVPacketToFrame(avPacket);

    //write_mtx_.Release();

    if (total_timer_.Get() < IGNORE_BEGIN_DATA_TIME)
    {
        SIMPLE_LOG("[%d] ignore begining data\n", source_id_);
        return;
    }

    if (video_arrive_timer_.LastTimestamp() < 0)
    {
        video_arrive_timer_.Reset();
    }

    if (outlen > 0 && observer_)
    {
        SIMPLE_LOG("[%d]\n", source_id_);
        // 解码出原始图像，回调
        long long diff_time = timestamp - video_last_timestamp_;
        observer_->OnSourceReaderVideoBuf(source_id_,
            h264_decoder_->GetDecodedFrame(), diff_time);
    }

    SIMPLE_LOG("[%d] end\n", source_id_);
}

void SourceReader::RecordAudio(const char* dataBuf, int dataSize,
    long long timestamp)
{
    if (IsThreadStop()) return;

    if (is_recording_ && recorder_ && src_cfg_.need_audio_)
    {
        recorder_->OnAACFrame(dataBuf, dataSize);
    }
}

char gH264StartCode[] = {0x00, 0x00, 0x00, 0x01};

void SourceReader::RecordVideo(const char* dataBuf, int dataSize,
    long long timestamp)
{
    if (IsThreadStop()) return;

    if (is_recording_ && recorder_)
    {
        recorder_->OnH264Frame(dataBuf, dataSize, false, timestamp);
    }
}

long long SourceReader::AudioArriveMillsecs()
{
    if (audio_arrive_timer_.LastTimestamp() < 0)
    {
        return 0;
    }

    return audio_arrive_timer_.Get();
}

long long SourceReader::VideoArriveMillsecs()
{
    if (video_arrive_timer_.LastTimestamp() < 0)
    {
        return 0;
    }

    return video_arrive_timer_.Get();
}

void SourceReader::OnCheckTimer()
{
    if (IsThreadStop()) return;

    SIMPLE_LOG("[%d] beg\n", source_id_);

    base::AutoLock al(write_mtx_);

    //SIMPLE_LOG("[%d]\n", source_id_);

    if (rtsp_reader_ && rtsp_reader_->IsConnectError())
    {
        SIMPLE_LOG("[%d]\n", source_id_);
        rtsp_reader_->Stop();
        delete rtsp_reader_;
        SIMPLE_LOG("[%d]\n", source_id_);

        audio_last_timestamp_ = -1;
        video_last_timestamp_ = -1;

        rtsp_reader_ = new FFUrlReader(this);
        rtsp_reader_->Start(source_url_.c_str());
        SIMPLE_LOG("[%d]\n", source_id_);
    }

    if (rtmp_reader_)
    {
        if (rtmp_reader_->IsNeedReConnected())
        {
            SIMPLE_LOG("[%d]\n", source_id_);

            rtmp_reader_->Stop();
            delete rtmp_reader_;

            SIMPLE_LOG("[%d]\n", source_id_);

            audio_last_timestamp_ = -1;
            video_last_timestamp_ = -1;

            rtmp_reader_ = new RtmpReader(false);
            rtmp_reader_->SetObserver(this);
            rtmp_reader_->Start(source_url_.c_str());
        }
        else if (rtmp_reader_->IsCodecError())
        {
            SIMPLE_LOG("[%d] 编码错误\n", source_id_);
        }
    }

    SIMPLE_LOG("[%d] end\n", source_id_);
}

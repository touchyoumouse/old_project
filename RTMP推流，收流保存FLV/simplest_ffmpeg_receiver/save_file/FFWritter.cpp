#include "FFWritter.h"

//#include "simple_logger.h"
//#include "gui/convert.h"

FFWritter::FFWritter(const std::string& fileName, const std::string& fmtName)
    : filename_(fileName), fmt_name_(fmtName)
{
    fmt_ctx_ = NULL;

    out_fmt_ = NULL;
    audio_stream_ = NULL;
    video_stream_ = NULL;

    audio_timebase_ = 0;
    video_timebase_ = 0;

    audio_pts_ = 0;
    audio_duration_ = 0;
    audio_frame_count_ = 0;
    video_pts_ = 0;
    video_duration_ = 0;

    last_pause_pts_ = -1;
    delayed_pts_ = 0;

    opening_pts_ = 0;
    opening_audio_pts_ = 0;

    last_video_pts_ = -1;

    audio_diff_millsec_ = 0;

    audio_total_size_ = 0;
}

FFWritter::~FFWritter()
{
    Close();
}

bool FFWritter::Open(bool isHasVideo, AVCodecID videoId,
    int width, int height, int fps, int bitrate,
    bool isHasAudio, AVCodecID audioId,
    int sampleRate, int channelCount, int audioBitrate)
{
	/* SIMPLE_LOG("width: %d, height: %d, bitrate: %d, samplerate: %d, channel: %d\n",\
		 width, height, bitrate, sampleRate, channelCount);*/

    video_codec_id_ = videoId;
    audio_codec_id_ = audioId;


	////解码初始化  
	//init_decode(audioId);  
	////编码初始化  
	//init_code(audioId);
    //av_register_all();
//     avformat_alloc_output_context2(&fmt_ctx_, NULL, NULL, "mp4");
//     if (!fmt_ctx_)
//     {
//         return false;
//     }
//     out_fmt_ = fmt_ctx_->oformat;

    fmt_ctx_ = avformat_alloc_context();
    out_fmt_ = av_guess_format(fmt_name_.c_str(), NULL, NULL);
    if (out_fmt_ == NULL)
    {
        printf("can not create media format: %s\n", fmt_name_.c_str());
        return false;
    }

   /* SIMPLE_LOG("\n");*/

    fmt_ctx_->oformat = out_fmt_;
    if (isHasVideo)
    {
        AddVideoStream(width, height, fps, bitrate);
        OpenVideo();
    }
    if (isHasAudio)
    {
        AddAudioStream(sampleRate, channelCount, audioBitrate);
        OpenAudio();
    }

   /* SIMPLE_LOG("avio open: %s\n", filename_.c_str());*/

    /* open the output file, if needed */
    if (!(out_fmt_->flags & AVFMT_NOFILE))
    {
        std::string utf8name = filename_/*framework::Wide2Utf8(framework::b2w(filename_))*/;
       /* SIMPLE_LOG("avio open utf8: %s\n", utf8name.c_str());*/
        int avio_err = avio_open(&fmt_ctx_->pb, utf8name.c_str(), AVIO_FLAG_WRITE);
        if (avio_err < 0)
        {
            char err_buf[1024] = {0};
            av_make_error_string(err_buf, 1024, avio_err);
           /* SIMPLE_LOG("false, err: %d, %s, last err: %d\n", avio_err, err_buf, GetLastError());*/
            return false;
        }
    }

   /* SIMPLE_LOG("write header\n");*/
    avformat_write_header(fmt_ctx_, NULL);

    audio_duration_ = (1000.0 / (sampleRate / 1024.0)) * 1000 * 90000 / AV_TIME_BASE;
    video_duration_ = (1000.0 / fps) * 1000 * 90000 / AV_TIME_BASE;

    audio_timebase_ = av_q2d(audio_stream_->time_base);
    video_timebase_ = av_q2d(video_stream_->time_base);

    return true;
}

void FFWritter::Close()
{
    if (fmt_ctx_ == NULL) return;

   /* SIMPLE_LOG("write trailer\n");*/

    av_write_trailer(fmt_ctx_);

  /*  SIMPLE_LOG("close codec\n");*/

    avcodec_close(audio_stream_->codec);
    avcodec_close(video_stream_->codec);

    /* free the streams */
    for(int i = 0; i < fmt_ctx_->nb_streams; i++)
    {
        av_freep(&fmt_ctx_->streams[i]->codec);
        av_freep(&fmt_ctx_->streams[i]);
    }

   /* SIMPLE_LOG("avio close\n");*/

    if (!(out_fmt_->flags & AVFMT_NOFILE))
    {
        avio_close(fmt_ctx_->pb);
    }

    av_free(fmt_ctx_);
    fmt_ctx_ = NULL;

  /*  SIMPLE_LOG("end\n");*/
}

unsigned int FFWritter::FileSize()
{
    if (fmt_ctx_ && fmt_ctx_->pb)
        return avio_size(fmt_ctx_->pb);
    else
        return 0;
}

void FFWritter::AddAudioStream(int sampleRate, int channelCount, int bitrate /* = 64000 */)
{
//    audio_stream_ = av_new_stream(fmt_ctx_, 1);
    audio_stream_ = avformat_new_stream(fmt_ctx_, NULL);

    AVCodecContext* codec_ctx = audio_stream_->codec;
    codec_ctx->codec_id = audio_codec_id_;
    codec_ctx->codec_type = AVMEDIA_TYPE_AUDIO;

    codec_ctx->sample_fmt = AV_SAMPLE_FMT_S16;
    codec_ctx->bit_rate = 19200/*3000*//*bitrate*/;
    codec_ctx->sample_rate = sampleRate;
    codec_ctx->channels = channelCount;

    if (fmt_ctx_->oformat->flags & AVFMT_GLOBALHEADER)
    {
        codec_ctx->flags |= CODEC_FLAG_GLOBAL_HEADER;
    }
}

void FFWritter::AddVideoStream(int width, int height, int fps, int bitrate)
{
    //video_stream_ = av_new_stream(fmt_ctx_, 0);
    video_stream_ = avformat_new_stream(fmt_ctx_, NULL);

    AVCodecContext* codec_ctx = video_stream_->codec;
    codec_ctx->codec_id = video_codec_id_;
    codec_ctx->codec_type = AVMEDIA_TYPE_VIDEO;

    codec_ctx->bit_rate = bitrate;
    codec_ctx->width = width;
    codec_ctx->height = height;
    codec_ctx->time_base.den = fps;
    codec_ctx->time_base.num = 1;
    codec_ctx->gop_size = 12; /* emit one intra frame every twelve frames at most */
    codec_ctx->pix_fmt = PIX_FMT_YUV420P;

    if (fmt_ctx_->oformat->flags & AVFMT_GLOBALHEADER)
    {
        codec_ctx->flags |= CODEC_FLAG_GLOBAL_HEADER;
    }
}

void FFWritter::OpenAudio()
{
    AVCodecContext* codec_ctx = audio_stream_->codec;

    AVCodec* audio_codec = avcodec_find_encoder(codec_ctx->codec_id);
    int ret = avcodec_open2(codec_ctx, audio_codec, NULL);
    int a = ret;
}

void FFWritter::OpenVideo()
{
    AVCodecContext* codec_ctx = video_stream_->codec;

    AVCodec* video_codec = avcodec_find_encoder(codec_ctx->codec_id);
    int ret = avcodec_open2(codec_ctx, video_codec, NULL);
    int a = ret;
}

void FFWritter::WriteAudioEncodedData(const char* dataBuf, int bufLen,
    long long delayMillsecs/* = 0*/)
{
    AVCodecContext* codec_ctx = audio_stream_->codec;
   /* SIMPLE_LOG("%s beg\n", fmt_name_.c_str());*/
//     {
//         long long now_time = av_gettime() - delayed_pts_;
//         if (video_pts_ == 0)
//         {
//             video_pts_ = now_time;
//         }
//     }
    
    //audio_frame_count_++;

    long long frame_pts = av_rescale_q(audio_frame_count_,
        av_make_q(1, codec_ctx->sample_rate), audio_stream_->time_base);
    frame_pts += av_rescale_q(delayMillsecs, av_make_q(1, 1000), audio_stream_->time_base);;

    {
        if (tc_.LastTimestamp() == -1) tc_.Reset();
        long long millsec = av_rescale_q(frame_pts, audio_stream_->time_base, av_make_q(1, 1000));

        audio_diff_millsec_ = (tc_.Get() - millsec);

       // SIMPLE_LOG("%s appending: %lld, pts: %lld, %lld\n", fmt_name_.c_str(), opening_audio_pts_, millsec, tc_.Get());
    }

    frame_pts = frame_pts + opening_audio_pts_;

    audio_frame_count_ += 1024;

    audio_total_size_ += bufLen;
    //SIMPLE_LOG("%s [Audio] frame_count: %lld, size: %lld\n", fmt_name_.c_str(), audio_frame_count_, audio_total_size_);

    WriteAudioFrame(dataBuf, bufLen, frame_pts);

   // SIMPLE_LOG("%s end\n", fmt_name_.c_str());
}

void FFWritter::WriteAudioFrame(const char* dataBuf, int bufLen,
    long long framePts)
{
    if (fmt_ctx_->pb == NULL) return;

    AVPacket av_pkt;
    av_init_packet(&av_pkt);

    av_pkt.flags |= AV_PKT_FLAG_KEY;
    av_pkt.stream_index = audio_stream_->index;
    av_pkt.data = (uint8_t *)dataBuf;
    av_pkt.size = bufLen;

    av_pkt.pts = framePts;
    av_pkt.dts = av_pkt.pts;

    /* write the compressed frame in the media file */
    int ret = av_interleaved_write_frame(fmt_ctx_, &av_pkt);
	//int ret = 0;
    //av_write_frame(fmt_ctx_, &av_pkt);
    if (ret != 0)
    {
        char errbuf[256] = {0};
        av_make_error_string(errbuf, 256, ret);
       // SIMPLE_LOG("%s write failed: %d, %s\n", fmt_name_.c_str(), ret, errbuf);
    }
}

void FFWritter::WriteVideoEncodedData(const char* dataBuf, int bufLen, bool isKeyframe,
    long long shouldTS)
{
    //SIMPLE_LOG("%s beg\n", fmt_name_.c_str());

    long long now_time = shouldTS - delayed_pts_;
    if (video_pts_ == 0)
    {
        video_pts_ = now_time;
    }

    long long frame_pts = (now_time-video_pts_/* - audio_diff_millsec_*1000*/)* 
        double(video_stream_->time_base.den) / AV_TIME_BASE + 0.5;
    
    frame_pts = frame_pts + opening_pts_;

    if (frame_pts == last_video_pts_)
    {
        frame_pts += 1;
    }

    last_video_pts_ = frame_pts;

    if (tc_.LastTimestamp() == -1) tc_.Reset();

   // SIMPLE_LOG("%s appending: %lld, pts: %lld, %lld\n", fmt_name_.c_str(), opening_pts_, (now_time-video_pts_)/1000, tc_.Get());
   // SIMPLE_LOG("%s frame pts: %lld, audio diff: %d\n", fmt_name_.c_str(), frame_pts, audio_diff_millsec_);

    WriteVideoFrame(dataBuf, bufLen, isKeyframe, frame_pts);

    //SIMPLE_LOG("%s end\n", fmt_name_.c_str());
}

void FFWritter::WriteVideoFrame(const char* dataBuf, int bufLen,
    bool isKeyframe, long long framePts)
{
    if (fmt_ctx_->pb == NULL) return;

    AVPacket av_pkt;
    av_init_packet(&av_pkt);

    if(isKeyframe)
        av_pkt.flags |= AV_PKT_FLAG_KEY;
    av_pkt.stream_index = video_stream_->index;
    av_pkt.data = (uint8_t *)dataBuf;
    av_pkt.size = bufLen;

    av_pkt.pts = framePts;
    av_pkt.dts = av_pkt.pts;

    /* write the compressed frame in the media file */
    int ret = av_interleaved_write_frame(fmt_ctx_, &av_pkt);
    //av_write_frame(fmt_ctx_, &av_pkt);
    if (ret != 0)
    {
        char errbuf[256] = {0};
        av_make_error_string(errbuf, 256, ret);
       // SIMPLE_LOG("%s write failed: %d, %s\n", fmt_name_.c_str(), ret, errbuf);
    }
}

void FFWritter::SetAudioExtraData(const char* extraData, int extraSize)
{
    AVCodecContext* codec_ctx = audio_stream_->codec;

    codec_ctx->extradata = (uint8_t*)av_malloc(extraSize + FF_INPUT_BUFFER_PADDING_SIZE);
    codec_ctx->extradata_size = extraSize;
    memcpy(codec_ctx->extradata, extraData, codec_ctx->extradata_size);
}

void FFWritter::SetVideoExtraData(const char* extraData, int extraSize)
{
    AVCodecContext* codec_ctx = video_stream_->codec;

    codec_ctx->extradata = (uint8_t*)av_malloc(extraSize);
    codec_ctx->extradata_size = extraSize;
    memcpy(codec_ctx->extradata, extraData, codec_ctx->extradata_size);
}

void FFWritter::SetPause(bool isPause)
{
    if (last_pause_pts_ >= 0)  // 暂停中
    {
        if (false == isPause)
        {
            // 取消暂停

            delayed_pts_ += (av_gettime()-last_pause_pts_);

            last_pause_pts_ = -1;
        }
    }
    else                       // 正常情况
    {
        if (isPause)
        {
            // 开始暂停

            last_pause_pts_ = av_gettime();
        }
    }
}

// 添加音频/视频片头数据
void FFWritter::WriteAudioOpeningData(const char* dataBuf, int bufLen,
    long long pts)
{
    if (pts == -1)
    {
        WriteAudioEncodedData(dataBuf, bufLen);
    }
    else
    {
        pts = av_rescale_q(pts, av_make_q(1, 1000), audio_stream_->time_base);

        //SIMPLE_LOG("%s appending: %lld, pts: %lld\n", fmt_name_.c_str(), opening_audio_pts_, pts);

        WriteAudioFrame(dataBuf, bufLen, pts);
    }
}

void FFWritter::WriteVideoOpeningData(const char* dataBuf, int bufLen,
    long long pts)
{
    pts = av_rescale_q(pts, av_make_q(1, 1000), video_stream_->time_base);
    //SIMPLE_LOG("%s appending: %lld, pts: %lld\n", fmt_name_.c_str(), opening_pts_, pts);
    WriteVideoFrame(dataBuf, bufLen, true, pts);
}

// 添加音频/视频片尾数据
void FFWritter::WriteAudioEndingData(const char* dataBuf, int bufLen,
    long long pts)
{
    //SIMPLE_LOG("\n");
    WriteAudioEncodedData(dataBuf, bufLen);
}

void FFWritter::WriteVideoEndingData(const char* dataBuf, int bufLen,
    long long pts)
{
    pts = av_rescale_q(pts, av_make_q(1, 1000), video_stream_->time_base);

    //SIMPLE_LOG("\n");
    WriteVideoFrame(dataBuf, bufLen, true, last_video_pts_ + pts);
}

void FFWritter::SetOpeningDuration(long long audioDuration,
    long long videoDuration)
{
    opening_audio_pts_ = av_rescale_q(audioDuration, av_make_q(1, 1000), audio_stream_->time_base);
    opening_pts_ = av_rescale_q(videoDuration, av_make_q(1, 1000), video_stream_->time_base);
}

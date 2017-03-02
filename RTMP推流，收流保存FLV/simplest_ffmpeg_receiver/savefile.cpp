
#include "savefile.h"



int savefile::Openfile(char* filepath)
{
	return 0;
}
int savefile::Closefile(char* filepath)
{
	return 0;
}
char* savefile::ReadFrame(int* readType, int* readSize, long long* frameTime,
	bool* isKeyframe)
{
	*readSize = 0;
	*readType = 0;

	av_init_packet(&reading_pakt_);

	if (av_read_frame(ff_fmt_ctx_, &reading_pakt_) < 0)
	{
		*readType = -1;

		return NULL;
	}

	bool has_got_keyframe = false;
	char* ret_buf = NULL;

	if (reading_pakt_.stream_index == audio_stream_index_)
	{
		// 音频数据
		if (reading_pakt_.size > 0/* && has_got_keyframe*/)
		{
			long long duration = reading_pakt_.duration * 1000.0 / audio_timebase_;

			*readType = 1;

			ret_buf = (char*)reading_pakt_.data;
			*readSize = reading_pakt_.size;
			*frameTime = a_timestamp_;

			a_timestamp_ += duration;

			a_timestamp_ = av_rescale_q(reading_pakt_.pts + reading_pakt_.duration,
				ff_fmt_ctx_->streams[audio_stream_index_]->time_base, av_make_q(1, 1000));
		}
	}
	else if (reading_pakt_.stream_index == video_stream_index_)
	{
		AVStream* video_stream = ff_fmt_ctx_->streams[video_stream_index_];

		// 视频数据
		if (false == has_got_keyframe)
		{
			has_got_keyframe = (reading_pakt_.flags & AV_PKT_FLAG_KEY);
		}

		if (reading_pakt_.size > 0/* && has_got_keyframe*/)
		{
			long long duration = reading_pakt_.duration * 1000.0 / video_timebase_;
			//duration *= 2;

			*readType = 2;

			ret_buf = (char*)reading_pakt_.data;
			*readSize = reading_pakt_.size;
			*frameTime = v_timestamp_;

			if (video_id_ == AV_CODEC_ID_H264)
			{
				v_timestamp_ += duration;

				av_bitstream_filter_filter(bsfc_, video_stream->codec, NULL,
					&reading_pakt_.data, &reading_pakt_.size, reading_pakt_.data, reading_pakt_.size, 0);

				ret_buf = (char*)reading_pakt_.data;
				*readSize = reading_pakt_.size;
			}
			else
			{
				v_timestamp_ = av_rescale_q(reading_pakt_.pts + reading_pakt_.duration,
					ff_fmt_ctx_->streams[video_stream_index_]->time_base, av_make_q(1, 1000));
			}

			if (isKeyframe)
			{
				*isKeyframe = ((reading_pakt_.flags & AV_PKT_FLAG_KEY) == AV_PKT_FLAG_KEY);
			}
		}
	}

	return ret_buf;
}
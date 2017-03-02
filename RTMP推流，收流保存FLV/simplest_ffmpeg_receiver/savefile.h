

//#include "stdafx"
#include <string>
//Windows
extern "C"
{
#include "libavformat/avformat.h"
#include "libavutil/mathematics.h"
#include "libavutil/time.h"
#include "libavdevice/avdevice.h" //camer
};

class savefile
{
public:

public:
	int	Openfile(char* filepath);

	int Closefile(char* filepath);

	char* ReadFrame(int* readType, int* readSize, long long* frameTime,
		bool* isKeyframe);

private:
	std::string filename_;

	AVFormatContext* ff_fmt_ctx_;
	int audio_stream_index_;
	int video_stream_index_;
	AVCodecID audio_id_;
	AVCodecID video_id_;

	AVBitStreamFilterContext* bsfc_;
	bool is_picture_;

	AVPacket reading_pakt_;

	int audio_timebase_;
	int video_timebase_;
	long long a_timestamp_;
	long long v_timestamp_;

	int width_;
	int height_;
	int sample_rate_;
	int channel_count_;
	long long audio_duration_;
	long long video_duration_;

	bool is_has_audio_;
	bool is_has_video_;

};
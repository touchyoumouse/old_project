///**
// * 最简单的基于FFmpeg的收流器（接收RTMP）
// * Simplest FFmpeg Receiver (Receive RTMP)
// * 
// * 雷霄骅 Lei Xiaohua
// * leixiaohua1020@126.com
// * 中国传媒大学/数字电视技术
// * Communication University of China / Digital TV Technology
// * http://blog.csdn.net/leixiaohua1020
// * 
// * 本例子将流媒体数据（以RTMP为例）保存成本地文件。
// * 是使用FFmpeg进行流媒体接收最简单的教程。
// *
// * This example saves streaming media data (Use RTMP as example)
// * as a local file.
// * It's the simplest FFmpeg stream receiver.
// * 
// */
//
//#include <stdio.h>
//
//#define __STDC_CONSTANT_MACROS
//
//#ifdef _WIN32
////Windows
//extern "C"
//{
//#include "libavformat/avformat.h"
//#include "libavutil/mathematics.h"
//#include "libavutil/time.h"
//};
//#else
////Linux...
//#ifdef __cplusplus
//extern "C"
//{
//#endif
//#include <libavformat/avformat.h>
//#include <libavutil/mathematics.h>
//#include <libavutil/time.h>
//#ifdef __cplusplus
//};
//#endif
//#endif
//
////'1': Use H.264 Bitstream Filter 
//#define USE_H264BSF 1
//
//int main(int argc, char* argv[])
//{
//	AVOutputFormat *ofmt = NULL;
//	//Input AVFormatContext and Output AVFormatContext
//	AVFormatContext *ifmt_ctx = NULL, *ofmt_ctx = NULL;
//	AVPacket pkt;
//	const char *in_filename, *out_filename;
//	int ret, i;
//	int videoindex=-1;
//	int frame_index=0;
//	//in_filename  = "rtmp://live.hkstv.hk.lxdns.com/live/hks";
//	in_filename = "rtmp://192.168.1.81:1935/live/90 live=1";
//	//in_filename  = "rtp://233.233.233.233:6666";
//	//out_filename = "receive.ts";
//	//out_filename = "receive.mkv";
//	//out_filename = "receive.flv";
//	out_filename = "receive.mp4";
//
//	av_register_all();
//	//Network
//	avformat_network_init();
//	//Input
//	if ((ret = avformat_open_input(&ifmt_ctx, in_filename, 0, 0)) < 0) {
//		printf( "Could not open input file.");
//		goto end;
//	}
//	if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0) {
//		printf( "Failed to retrieve input stream information");
//		goto end;
//	}
//
//	for(i=0; i<ifmt_ctx->nb_streams; i++) 
//		if(ifmt_ctx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO){
//			videoindex=i;
//			break;
//		}
//
//	av_dump_format(ifmt_ctx, 0, in_filename, 0);
//
//
//	//Output
//	// avformat_alloc_output_context2()函数可以初始化一个用于输出的AVFormatContext结构体
//	avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, out_filename); //RTMP
//
//	if (!ofmt_ctx) {
//		printf( "Could not create output context\n");
//		ret = AVERROR_UNKNOWN;
//		goto end;
//	}
//	ofmt = ofmt_ctx->oformat;
//	for (i = 0; i < ifmt_ctx->nb_streams; i++) {
//		//Create output AVStream according to input AVStream
//		AVStream *in_stream = ifmt_ctx->streams[i];
//		AVStream *out_stream = avformat_new_stream(ofmt_ctx, in_stream->codec->codec);
//		if (!out_stream) {
//			printf( "Failed allocating output stream\n");
//			ret = AVERROR_UNKNOWN;
//			goto end;
//		}
//		//Copy the settings of AVCodecContext
//		ret = avcodec_copy_context(out_stream->codec, in_stream->codec);
//		if (ret < 0) {
//			printf( "Failed to copy context from input to output stream codec context\n");
//			goto end;
//		}
//		out_stream->codec->codec_tag = 0;
//		if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
//			out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
//	}
//	//Dump Format------------------
//	av_dump_format(ofmt_ctx, 0, out_filename, 1);
//	//Open output URL
//	//如果设置了AVFMT_NOFILE  则pb会另有所取，avio_open会返回NULL
//	if (!(ofmt->flags & AVFMT_NOFILE)) {
//		ret = avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE);
//		if (ret < 0) {
//			printf( "Could not open output URL '%s'", out_filename);
//			goto end;
//		}
//	}
//	//Write file header
//	ret = avformat_write_header(ofmt_ctx, NULL);
//	if (ret < 0) {
//		printf( "Error occurred when opening output URL\n");
//		goto end;
//	}
//
//#if USE_H264BSF
//	AVBitStreamFilterContext* h264bsfc =  av_bitstream_filter_init("h264_mp4toannexb"); 
//#endif
//	int num = 0;
//	while (1) {
//		AVStream *in_stream, *out_stream;
//		//Get an AVPacket
//		ret = av_read_frame(ifmt_ctx, &pkt);
//		if (ret < 0)
//			break;
//		
//		in_stream  = ifmt_ctx->streams[pkt.stream_index];
//		out_stream = ofmt_ctx->streams[pkt.stream_index];
//		/* copy packet */
//		//Convert PTS/DTS
//		pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
//		pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
//		pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
//		pkt.pos = -1;
//		//Print to Screen
//		if(pkt.stream_index==videoindex){
//			printf("Receive %8d video frames from input URL\n",frame_index);
//			frame_index++;
//
//#if USE_H264BSF
//			av_bitstream_filter_filter(h264bsfc, in_stream->codec, NULL, &pkt.data, &pkt.size, pkt.data, pkt.size, 0);
//#endif
//		}
//		//ret = av_write_frame(ofmt_ctx, &pkt);
//		ret = av_interleaved_write_frame(ofmt_ctx, &pkt);
//
//		if (ret < 0) {
//			printf( "Error muxing packet\n");
//			break;
//		}
//		
//		av_free_packet(&pkt);
//		num++;
//		if (num == 1000)
//		{
//	//		break;
//		}
//	}
//
//#if USE_H264BSF
//	av_bitstream_filter_close(h264bsfc);  
//#endif
//
//	//Write file trailer
//	av_write_trailer(ofmt_ctx);
//end:
//	avformat_close_input(&ifmt_ctx);
//	/* close output */
//	if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
//		avio_close(ofmt_ctx->pb);
//	avformat_free_context(ofmt_ctx);
//	if (ret < 0 && ret != AVERROR_EOF) {
//		printf( "Error occurred.\n");
//		return -1;
//	}
//	return 0;
//}
//
//
//
/**
* 最简单的基于FFmpeg的收流器（接收RTMP）
* Simplest FFmpeg Receiver (Receive RTMP)
*
* 雷霄骅 Lei Xiaohua
* leixiaohua1020@126.com
* 中国传媒大学/数字电视技术
* Communication University of China / Digital TV Technology
* http://blog.csdn.net/leixiaohua1020
*
* 本例子将流媒体数据（以RTMP为例）保存成本地文件。
* 是使用FFmpeg进行流媒体接收最简单的教程。
*
* This example saves streaming media data (Use RTMP as example)
* as a local file.
* It's the simplest FFmpeg stream receiver.
*
*/

#include <stdio.h>
#include <deque>
#define __STDC_CONSTANT_MACROS

#ifdef _WIN32
//Windows
extern "C"
{
#include "libavformat/avformat.h"
#include "libavutil/mathematics.h"
#include "libavutil/time.h"
};


extern "C" {
#include "libavcodec/avcodec.h"
#include "libswresample/swresample.h"
#include "libavutil/opt.h"
}

extern "C" {

#include "libswscale/swscale.h"

}


#else
//Linux...
#ifdef __cplusplus
extern "C"
{
#endif
#include <libavformat/avformat.h>
#include <libavutil/mathematics.h>
#include <libavutil/time.h>
#ifdef __cplusplus
};
#endif
#endif

std::deque<char*> video_bufs_cache;

//'1': Use H.264 Bitstream Filter 
#define USE_H264BSF 1

void frame_info(AVPacket *,int);

AVCodecContext* ff_codec_ctx_;
AVCodec *pCodec;



int main(int argc, char* argv[])
{


	AVOutputFormat *ofmt = NULL;
	//Input AVFormatContext and Output AVFormatContext
	AVFormatContext *ifmt_ctx = NULL, *ofmt_ctx = NULL;
	AVPacket pkt;
	const char *in_filename, *out_filename;
	int ret, i;
	int videoindex = -1;
	int frame_index = 0;
	in_filename  = "rtmp://live.hkstv.hk.lxdns.com/live/hks";
	//in_filename = "rtmp://192.168.1.81:1935/live/90 live=1";
	//in_filename  = "rtp://233.233.233.233:6666";
	//out_filename = "receive.ts";
	//out_filename = "receive.mkv";
	//out_filename = "receive.flv";
	out_filename = "receive.mp4";

	av_register_all();
	//Network
	avformat_network_init();
	//Input
	if ((ret = avformat_open_input(&ifmt_ctx, in_filename, 0, 0)) < 0) {
		printf("Could not open input file.");
		goto end;
	}
	if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0) {
		printf("Failed to retrieve input stream information");
		goto end;
	}

	for (i = 0; i<ifmt_ctx->nb_streams; i++)
	if (ifmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO){
		videoindex = i;

		/*pCodec = avcodec_find_decoder(ifmt_ctx->streams[i]->codec->codec_id);
		ff_codec_ctx_ = avcodec_alloc_context3(NULL);
		if (avcodec_open2(ff_codec_ctx_, pCodec, NULL) < 0)
		{
		int a = 1;
		}*/
		break;
	}

	av_dump_format(ifmt_ctx, 0, in_filename, 0);

	//Output
	// avformat_alloc_output_context2()函数可以初始化一个用于输出的AVFormatContext结构体
	avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, out_filename); //RTMP

	if (!ofmt_ctx) {
		printf("Could not create output context\n");
		ret = AVERROR_UNKNOWN;
		goto end;
	}
	ofmt = ofmt_ctx->oformat;
	for (i = 0; i < ifmt_ctx->nb_streams; i++) {
		//Create output AVStream according to input AVStream
		AVStream *in_stream = ifmt_ctx->streams[i];
		AVStream *out_stream = avformat_new_stream(ofmt_ctx, in_stream->codec->codec);
		if (!out_stream) {
			printf("Failed allocating output stream\n");
			ret = AVERROR_UNKNOWN;
			goto end;
		}
		//Copy the settings of AVCodecContext
		ret = avcodec_copy_context(out_stream->codec, in_stream->codec);
		if (ifmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			ff_codec_ctx_ = avcodec_alloc_context3(NULL);
			ret = avcodec_copy_context(ff_codec_ctx_, in_stream->codec);
			pCodec = avcodec_find_decoder(ifmt_ctx->streams[i]->codec->codec_id);
			if (avcodec_open2(ff_codec_ctx_, pCodec, NULL) < 0)
			{
				int a = 1;
			}
		}
		
		if (ret < 0) {
			printf("Failed to copy context from input to output stream codec context\n");
			goto end;
		}
		out_stream->codec->codec_tag = 0;
		if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
			out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
	}
	//Dump Format------------------
	av_dump_format(ofmt_ctx, 0, out_filename, 1);
	//Open output URL
	//如果设置了AVFMT_NOFILE  则pb会另有所取，avio_open会返回NULL
	if (!(ofmt->flags & AVFMT_NOFILE)) {
		ret = avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE);
		if (ret < 0) {
			printf("Could not open output URL '%s'", out_filename);
			goto end;
		}
	}
	//Write file header
	ret = avformat_write_header(ofmt_ctx, NULL);
	if (ret < 0) {
		printf("Error occurred when opening output URL\n");
		goto end;
	}

#if USE_H264BSF
	AVBitStreamFilterContext* h264bsfc = av_bitstream_filter_init("h264_mp4toannexb");
#endif
	int num = 0;
	while (1) {
		AVStream *in_stream, *out_stream;
		//Get an AVPacket
		ret = av_read_frame(ifmt_ctx, &pkt);
		if (ret < 0)
			break;
		
		frame_info(&pkt,videoindex);


		in_stream = ifmt_ctx->streams[pkt.stream_index];
		out_stream = ofmt_ctx->streams[pkt.stream_index];
		/* copy packet */
		//Convert PTS/DTS
		pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
		pkt.pos = -1;
		//Print to Screen
		if (pkt.stream_index == videoindex){
			printf("Receive %8d video frames from input URL\n", frame_index);
			frame_index++;

#if USE_H264BSF
			av_bitstream_filter_filter(h264bsfc, in_stream->codec, NULL, &pkt.data, &pkt.size, pkt.data, pkt.size, 0);
#endif
		}
		//ret = av_write_frame(ofmt_ctx, &pkt);
		ret = av_interleaved_write_frame(ofmt_ctx, &pkt);

		if (ret < 0) {
			printf("Error muxing packet\n");
			break;
		}

		av_free_packet(&pkt);
		num++;
		if (num == 1000)
		{
			//		break;
		}
	}

#if USE_H264BSF
	av_bitstream_filter_close(h264bsfc);
#endif

	//Write file trailer
	av_write_trailer(ofmt_ctx);
end:
	avformat_close_input(&ifmt_ctx);
	/* close output */
	if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
		avio_close(ofmt_ctx->pb);
	avformat_free_context(ofmt_ctx);
	if (ret < 0 && ret != AVERROR_EOF) {
		printf("Error occurred.\n");
		return -1;
	}
	return 0;
}


void frame_info(AVPacket* avpacket,int videoindex)
{
	if (avpacket->stream_index == videoindex)
	{
		/*if (avpacket->flags & AV_PKT_FLAG_KEY)
		{*/
			AVFrame*  avs_frame;
			AVFrame*  avs_YUVframe;
			avs_frame = av_frame_alloc();
			avs_YUVframe = av_frame_alloc();
			uint8_t *out_buffer;
			
			
			//decode
			int num = 0;
			int av_num = avcodec_decode_video2(ff_codec_ctx_, avs_frame, &num, avpacket);
			int a = avpacket->buf->size;
			if (!(avs_frame->pict_type == AV_PICTURE_TYPE_NONE))
			{
				int pktsize = avpacket->size;
				int keyframe = avs_frame->key_frame;
				int width = avs_frame->width;
				int hight = avs_frame->height;
				printf("keyframe = %d \ n", keyframe);
				printf("width = %d \n", width);
				printf("hight = %d \n", hight);
				printf("pktsize = %d \n", pktsize);
				if (1)
				{
					int num = 1;
					FILE *testfile = fopen("test2.rgb", "wb");
					
					out_buffer = new uint8_t[avpicture_get_size(AV_PIX_FMT_RGB24, ff_codec_ctx_->width, ff_codec_ctx_->height)];
					avpicture_fill((AVPicture *)avs_YUVframe, out_buffer, AV_PIX_FMT_RGB24, ff_codec_ctx_->width, ff_codec_ctx_->height);
					struct SwsContext * img_convert_ctx;
					img_convert_ctx = sws_getContext(ff_codec_ctx_->width, ff_codec_ctx_->height, ff_codec_ctx_->pix_fmt, ff_codec_ctx_->width, ff_codec_ctx_->height, AV_PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL);
					sws_scale(img_convert_ctx, (const uint8_t* const*)avs_frame->data, avs_frame->linesize, 0, ff_codec_ctx_->height, avs_YUVframe->data, avs_YUVframe->linesize);
					//RGB  
					fwrite(avs_YUVframe->data[0], (ff_codec_ctx_->width)*(ff_codec_ctx_->height) * 3, 1, testfile);
					//return;
					/*fwrite(avs_YUVframe->data[0], ff_codec_ctx_->width*ff_codec_ctx_->height, 1, testfile);
					fwrite(avs_YUVframe->data[1], ff_codec_ctx_->width*ff_codec_ctx_->height / 4, 1, testfile);
					fwrite(avs_YUVframe->data[2], ff_codec_ctx_->width*ff_codec_ctx_->height / 4, 1, testfile);*/
					fflush(testfile);
					fclose(testfile);
				}
			}
	/*	}*/
		char *video_frame = new char(avpacket->size);
		video_bufs_cache.push_back(video_frame);
	}
}











////////////////////////////////////////

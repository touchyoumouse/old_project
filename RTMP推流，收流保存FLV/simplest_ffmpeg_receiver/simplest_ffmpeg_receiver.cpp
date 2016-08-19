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
#include "librtmp/rtmp.h"
#include "librtmp/rtmp_sys.h"

typedef INT8 hb_int8;
typedef INT16 hb_int16;
typedef INT32 hb_int24;
typedef INT32 hb_int32;
typedef INT64 hb_int64;
typedef UINT8 hb_uint8;
typedef UINT16 hb_uint16;
typedef UINT32 hb_uint24;
typedef UINT32 hb_uint32;
typedef UINT64 hb_uint64;
char* sps_;        // sequence parameter set
int sps_size_;
char* pps_;        // picture parameter set
int pps_size_;
int timestamp = 0;


//FILE *fpSave = fopen("geth264.h264", "ab");




void my_logoutput(void* ptr, int level, const char* fmt,va_list vl){  
	FILE *fp = fopen("my_log.txt","a+");     
	if(fp){     
		vfprintf(fp,fmt,vl);  
		fflush(fp);  
		fclose(fp);  
	}     
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
bool isKeyframe = false;
AVCodecContext* ff_codec_ctx_;
AVCodec *pCodec;
RTMP *rtmp;
int lib_rtmp = 0;
AVCodecContext* ff_encodec_ctx_;
AVPacket *avpacket_rtmp;

int init_RTMP(char * stream_url_)
{
	{
		WORD version;
		WSADATA wsaData;
		version = MAKEWORD(2, 2);
		WSAStartup(version, &wsaData);
	}
	rtmp = RTMP_Alloc();
	RTMP_Init(rtmp);
	RTMP_SetBufferMS(rtmp, 300);
	int err = RTMP_SetupURL(rtmp, stream_url_);
	if (err <= 0) return false;

	RTMP_EnableWrite(rtmp);

	err = RTMP_Connect(rtmp, NULL);
	if (err <= 0) return false;

	err = RTMP_ConnectStream(rtmp, 0);
	if (err <= 0) return false;

	rtmp->m_outChunkSize = 1024;
	//SendSetChunkSize(rtmp->m_outChunkSize);
}


bool Send(const char* buf, int bufLen, int type, unsigned int timestamp)
{
	RTMPPacket rtmp_pakt;
	RTMPPacket_Reset(&rtmp_pakt);
	RTMPPacket_Alloc(&rtmp_pakt, bufLen);

	rtmp_pakt.m_packetType = type;
	rtmp_pakt.m_nBodySize = bufLen;
	rtmp_pakt.m_nTimeStamp = timestamp + lib_rtmp;
	lib_rtmp++;
	rtmp_pakt.m_nChannel = 4;
	rtmp_pakt.m_headerType = RTMP_PACKET_SIZE_LARGE;
	rtmp_pakt.m_nInfoField2 = rtmp->m_stream_id;

	memcpy(rtmp_pakt.m_body, buf, bufLen);

	int retval = RTMP_SendPacket(rtmp, &rtmp_pakt, 0);
	RTMPPacket_Free(&rtmp_pakt);

	return !!retval;
}
char* UI08ToBytes(char* buf, hb_uint8 val)
{
	char* pbuf = buf;
	buf[0] = (char)(val)& 0xff;
	pbuf += 1;
	return pbuf;
}
char* UI16ToBytes(char* buf, hb_uint16 val)
{
	char* pbuf = buf;
	buf[0] = (char)(val >> 8) & 0xff;
	buf[1] = (char)(val)& 0xff;
	pbuf += 2;
	return pbuf;
}
char* UI24ToBytes(char* buf, hb_uint24 val)
{
	char* pbuf = buf;
	buf[0] = (char)(val >> 16) & 0xff;
	buf[1] = (char)(val >> 8) & 0xff;
	buf[2] = (char)(val)& 0xff;
	pbuf += 3;
	return pbuf;
}

void SendAVCSequenceHeaderPacket()
{
	char avc_seq_buf[4096];

	char* pbuf = avc_seq_buf;

	unsigned char flag = 0;
	flag = (1 << 4) |   // frametype "1  == keyframe"
		7;              // codecid   "7  == AVC"

	pbuf = UI08ToBytes(pbuf, flag);

	pbuf = UI08ToBytes(pbuf, 0);    // avc packet type (0, header)
	pbuf = UI24ToBytes(pbuf, 0);    // composition time

	// AVCDecoderConfigurationRecord

	pbuf = UI08ToBytes(pbuf, 1);            // configurationVersion
	pbuf = UI08ToBytes(pbuf, sps_[1]);      // AVCProfileIndication
	pbuf = UI08ToBytes(pbuf, sps_[2]);      // profile_compatibility
	pbuf = UI08ToBytes(pbuf, sps_[3]);      // AVCLevelIndication
	pbuf = UI08ToBytes(pbuf, 0xff);         // 6 bits reserved (111111) + 2 bits nal size length - 1
	pbuf = UI08ToBytes(pbuf, 0xe1);         // 3 bits reserved (111) + 5 bits number of sps (00001)
	pbuf = UI16ToBytes(pbuf, sps_size_);    // sps
	memcpy(pbuf, sps_, sps_size_);
	pbuf += sps_size_;
	pbuf = UI08ToBytes(pbuf, 1);            // number of pps
	pbuf = UI16ToBytes(pbuf, pps_size_);    // pps
	memcpy(pbuf, pps_, pps_size_);
	pbuf += pps_size_;

	Send(avc_seq_buf, (int)(pbuf - avc_seq_buf), 0x09, 0);
}



int main(int argc, char* argv[])
{
	av_log_set_callback(my_logoutput);
	sps_ = new char[1024];
	sps_size_ = 0;
	pps_ = new char[1024];
	pps_size_ = 0;
	//////////////////////////////////////////////////////////////////////////
	AVOutputFormat *ofmt = NULL;
	//Input AVFormatContext and Output AVFormatContext
	AVFormatContext *ifmt_ctx = NULL, *ofmt_ctx = NULL;
	AVPacket pkt;
	 char *in_filename, *out_filename, *rtmp_url;
	int ret, i;
	int videoindex = -1;
	int frame_index = 0;
	in_filename  = "rtmp://live.hkstv.hk.lxdns.com/live/hks";
	rtmp_url = "rtmp://127.0.0.1/live/123";
	init_RTMP(rtmp_url);
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
		/////////////////////////////pps sps/////////////////////////////////////////////
		for (int i = 0; i < ifmt_ctx->streams[0]->codec->extradata_size; i++)
		{
			printf("%x ", ifmt_ctx->streams[0]->codec->extradata[i]);
		}
		sps_size_ = ifmt_ctx->streams[0]->codec->extradata[6] * 0xFF + ifmt_ctx->streams[0]->codec->extradata[7];
		pps_size_ = ifmt_ctx->streams[0]->codec->extradata[8 + sps_size_ + 1] * 0xFF + ifmt_ctx->streams[0]->codec->extradata[8 + sps_size_ + 2];
		for (int i = 0; i < sps_size_; i++)
		{
			sps_[i] = ifmt_ctx->streams[0]->codec->extradata[i + 8];
		}

		for (int i = 0; i < pps_size_; i++)
		{
			pps_[i] = ifmt_ctx->streams[0]->codec->extradata[i + 8 + 2 + 1 + sps_size_];
		}
		///////////////////////////////set encode///////////////////////////////////////////
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
			//ff_encodec_ctx_ = avcodec_alloc_context3(NULL);
			ret = avcodec_copy_context(ff_codec_ctx_, in_stream->codec);
			//avcodec_copy_context(ff_encodec_ctx_, in_stream->codec);
			pCodec = avcodec_find_decoder(ifmt_ctx->streams[i]->codec->codec_id);
			//avcodec_find_encoder(ifmt_ctx->streams[i]->codec->codec_id);

			AVCodec *pCodec1 = avcodec_find_encoder(AV_CODEC_ID_H264);
			ff_encodec_ctx_ = avcodec_alloc_context3(pCodec1);

			{
				/* put sample parameters */
				ff_encodec_ctx_->bit_rate = 400000;
				/* resolution must be a multiple of two */
				ff_encodec_ctx_->width = ff_codec_ctx_->width;
				ff_encodec_ctx_->height = ff_codec_ctx_->height;
				/* frames per second */
				ff_encodec_ctx_->time_base.den = 25;
				ff_encodec_ctx_->time_base.num = 1;
				ff_encodec_ctx_->gop_size = 10; /* emit one intra frame every ten frames */
				ff_encodec_ctx_->max_b_frames = 1;
				ff_encodec_ctx_->pix_fmt = AV_PIX_FMT_YUV420P;
				av_opt_set(ff_encodec_ctx_->priv_data, "preset", "superfast", 0);
				av_opt_set(ff_encodec_ctx_->priv_data, "tune", "zerolatency", 0);
			}
			if (avcodec_open2(ff_codec_ctx_, pCodec, NULL) < 0)
			{
				int a = 1;
			}
		
			if (avcodec_open2(ff_encodec_ctx_, pCodec1, NULL) < 0)
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
	FILE *fpSave;
	if ((fpSave = fopen("mytest1.h264", "ab+")) == NULL) //h264保存的文件名  
		return 0;
	unsigned char *dummy = NULL;   //输入的指针  
	int dummy_len;
	AVBitStreamFilterContext* bsfc = av_bitstream_filter_init("h264_mp4toannexb");
	av_bitstream_filter_filter(bsfc, ff_codec_ctx_, NULL, &dummy, &dummy_len, NULL, 0, 0);
	fwrite(ff_codec_ctx_->extradata, ff_codec_ctx_->extradata_size, 1, fpSave);
	av_bitstream_filter_close(bsfc);
	free(dummy);

	for (int i = 0;i < 500;i++)
	{
		//------------------------------  
		if (av_read_frame(ifmt_ctx, &pkt) >= 0)
		{
			if (pkt.stream_index == videoindex)
			{
				char nal_start[] = { 0, 0, 0, 1 };
				fwrite(nal_start, 4, 1, fpSave);
				fwrite(pkt.data + 4, pkt.size - 4, 1, fpSave);

				//fwrite(pkt.data, 1, pkt.size, fpSave);//写数据到文件中  
			}
			av_free_packet(&pkt);
		}
	}
	fclose(fpSave);
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



void SendVideoData(char* buf, int bufLen, unsigned int timestamp, bool isKeyframe)
{
	//FILE_LOG_DEBUG("size: %d, timestamp: %d\n", bufLen, timestamp);

	char* pbuf = buf;

	unsigned char flag = 0;
	if (isKeyframe)
		flag = 0x17;
	else
		flag = 0x27;

	pbuf = UI08ToBytes(pbuf, flag);

	pbuf = UI08ToBytes(pbuf, 1);    // avc packet type (0, nalu)
	pbuf = UI24ToBytes(pbuf, 0);    // composition time
	//timestamp -= i_video_timestamp;
	bool isok = Send(buf, bufLen, 0x09, timestamp);
	timestamp += 1000;
	if (false == isok)
	{
	}
	//i_video_timestamp += 1000;
}




void frame_info(AVPacket* avpacket,int videoindex)
{
	if (avpacket->stream_index == videoindex)
	{
		/*if (avpacket->flags & AV_PKT_FLAG_KEY)
		{*/
		//fwrite(avpacket->data, 1, avpacket->size, fpSave);//写数据到文件中  
			AVFrame*  avs_frame;
			AVFrame*  avs_YUVframe;
			avs_frame = av_frame_alloc();
			avs_YUVframe = av_frame_alloc();
			uint8_t *out_buffer;
			//decode
			int num = 0;
			int av_num = avcodec_decode_video2(ff_codec_ctx_, avs_frame, &num, avpacket);
			



			SendAVCSequenceHeaderPacket();
			int a = avpacket->buf->size;
			if (!(avs_frame->pict_type == AV_PICTURE_TYPE_NONE))
			{
				//SendAVCSequenceHeaderPacket();
				if (avs_frame->pict_type == AV_PKT_FLAG_KEY)
				{
					SendAVCSequenceHeaderPacket();
					isKeyframe = true;
					
				}
				//uint8_t *buf = (uint8_t *)malloc(*avs_frame->pkt_size);
				//avcodec_encode_video(ff_encodec_ctx_, buf, *avs_frame->linesize, avs_frame);
				
				
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
					//int num = 1;
					//FILE *testfile = fopen("test2.rgb", "wb");
					
					out_buffer = new uint8_t[avpicture_get_size(AV_PIX_FMT_YUV420P, ff_codec_ctx_->width, ff_codec_ctx_->height)];
					avpicture_fill((AVPicture *)avs_YUVframe, out_buffer, AV_PIX_FMT_YUV420P, ff_codec_ctx_->width, ff_codec_ctx_->height);
					struct SwsContext * img_convert_ctx;
					img_convert_ctx = sws_getContext(ff_codec_ctx_->width, ff_codec_ctx_->height, ff_codec_ctx_->pix_fmt, ff_codec_ctx_->width, ff_codec_ctx_->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
					sws_scale(img_convert_ctx, (const uint8_t* const*)avs_frame->data, avs_frame->linesize, 0, ff_codec_ctx_->height, avs_YUVframe->data, avs_YUVframe->linesize);
					//RGB  
					//fwrite(avs_YUVframe->data[0], (ff_codec_ctx_->width)*(ff_codec_ctx_->height) * 3, 1, testfile);
					
					AVPacket av_pakt;
					av_init_packet(&av_pakt);
					//strcpy(av_pakt.data, avs_YUVframe->data);
					av_pakt.size = avpicture_get_size(ff_encodec_ctx_->pix_fmt, ff_encodec_ctx_->width, ff_encodec_ctx_->height);
					//av_pakt.size = ff_encodec_ctx_->height*ff_encodec_ctx_->width*3/2;
					//uint8_t *picture_buf = (uint8_t *)av_malloc(av_pakt.size);
					//av_pakt.size = 0;
					av_pakt.data = NULL;
					//av_pakt.size = sizeof(AVPicture);
					//avpicture_fill((AVPicture *)avs_YUVframe, avs_YUVframe, ff_encodec_ctx_->pix_fmt, ff_encodec_ctx_->width, ff_encodec_ctx_->height);
					int got_packet_ptr = 0;
					int anv = avcodec_encode_video2(ff_encodec_ctx_, &av_pakt, avs_YUVframe, &got_packet_ptr);


					/*AVFrame  *testframe = NULL;
					testframe = av_frame_alloc();
					avcodec_decode_video2(ff_codec_ctx_, testframe, &num, &av_pakt);
					FILE *testfile = fopen("test3.yuv", "wb");
					fwrite(avs_YUVframe->data[0], ff_codec_ctx_->width*ff_codec_ctx_->height, 1, testfile);
					fwrite(avs_YUVframe->data[1], ff_codec_ctx_->width*ff_codec_ctx_->height / 4, 1, testfile);
					fwrite(avs_YUVframe->data[2], ff_codec_ctx_->width*ff_codec_ctx_->height / 4, 1, testfile);
					fflush(testfile);
				    fclose(testfile);*/


					if (av_pakt.data)
					    SendVideoData((char*)av_pakt.data, av_pakt.size, timestamp, isKeyframe);
					timestamp += 1000;
					//return;
					/*fwrite(avs_YUVframe->data[0], ff_codec_ctx_->width*ff_codec_ctx_->height, 1, testfile);
					fwrite(avs_YUVframe->data[1], ff_codec_ctx_->width*ff_codec_ctx_->height / 4, 1, testfile);
					fwrite(avs_YUVframe->data[2], ff_codec_ctx_->width*ff_codec_ctx_->height / 4, 1, testfile);*/
					//fflush(testfile);
					//fclose(testfile);
				}
			}
	/*	}*/
			
		char *video_frame = new char(avpacket->size);
		video_bufs_cache.push_back(video_frame);
		av_frame_free(&avs_frame);
		av_frame_free(&avs_YUVframe);
	}
}











////////////////////////////////////////

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
#include "encodec.h"
#include <stdio.h>
#include <deque>
#include <vector>
#include <list>
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
std::list<AVFrame*> video_frame_list_;

//FILE *fpSave = fopen("geth264.h264", "ab");


char* UI32ToBytes(char* buf, hb_uint32 val)
{
	char* pbuf = buf;
	buf[0] = (char)(val >> 24) & 0xff;
	buf[1] = (char)(val >> 16) & 0xff;
	buf[2] = (char)(val >> 8) & 0xff;
	buf[3] = (char)(val)& 0xff;
	pbuf += 4;
	return pbuf;
}

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
void SendSetChunkSize(unsigned int chunkSize);
std::deque<AVPacket* > video_bufs_cache;
//'1': Use H.264 Bitstream Filter 
#define USE_H264BSF 1
void frame_info();
bool isKeyframe = false;
AVCodecContext* ff_codec_ctx_;
AVCodec *pCodec;
RTMP *rtmp;
int lib_rtmp = 0;
AVCodecContext* ff_encodec_ctx_;
AVPacket *avpacket_rtmp;
X264Encoder* x264_encoder_;
RTMPPacket *rtmp_pakt;
char* live_264buf_;
int live_264size_;

void beginthread_fun(void *a)
{
	frame_info();
}


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
	//RTMP_SetBufferMS(rtmp, 300);
	RTMP_SetBufferMS(rtmp, 60 * 1000);
	rtmp->Link.lFlags |= RTMP_LF_LIVE;
	int err = RTMP_SetupURL(rtmp, stream_url_);
	if (err <= 0) return false;

	RTMP_EnableWrite(rtmp);

	err = RTMP_Connect(rtmp, NULL);
	if (err <= 0) return false;

	err = RTMP_ConnectStream(rtmp, 0);
	if (err <= 0) return false;

	rtmp->m_outChunkSize = 1024;
	SendSetChunkSize(rtmp->m_outChunkSize);
}
void SendSetChunkSize(unsigned int chunkSize)
{
	RTMPPacket rtmp_pakt;
	RTMPPacket_Reset(&rtmp_pakt);
	RTMPPacket_Alloc(&rtmp_pakt, 4);

	rtmp_pakt.m_packetType = 0x01;
	rtmp_pakt.m_nChannel = 0x02;    // control channel
	rtmp_pakt.m_headerType = RTMP_PACKET_SIZE_LARGE;
	rtmp_pakt.m_nInfoField2 = 0;


	rtmp_pakt.m_nBodySize = 4;
	UI32ToBytes(rtmp_pakt.m_body, chunkSize);

	RTMP_SendPacket(rtmp, &rtmp_pakt, 0);
	RTMPPacket_Free(&rtmp_pakt);
}

bool Send(const char* buf, int bufLen, int type, unsigned int timestamp)
{
	
	rtmp_pakt = (RTMPPacket*)malloc(sizeof(RTMPPacket));
	RTMPPacket_Alloc(rtmp_pakt, 1024 * 1024);
	RTMPPacket_Reset(rtmp_pakt);

	rtmp_pakt->m_packetType = type;
	rtmp_pakt->m_nBodySize = bufLen;
	rtmp_pakt->m_nTimeStamp = timestamp + lib_rtmp;
	lib_rtmp++;
	rtmp_pakt->m_nChannel = 0x04;
	rtmp_pakt->m_headerType = RTMP_PACKET_SIZE_LARGE;
	rtmp_pakt->m_nInfoField2 = rtmp->m_stream_id;

	memcpy(rtmp_pakt->m_body, buf, bufLen);

	if (!RTMP_IsConnected(rtmp)){
		//RTMP_Log(RTMP_LOGERROR, "rtmp is not connect\n");
		//return false;
		int a = 10;

	}

	int retval = RTMP_SendPacket(rtmp, rtmp_pakt, 0);
 	RTMPPacket_Free(rtmp_pakt);
	free(rtmp_pakt);
	rtmp_pakt = NULL;

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
#define RTMP_HEAD_SIZE   (sizeof(RTMPPacket)+RTMP_MAX_HEADER_SIZE)

void SendAVCSequenceHeaderPacket()
{
	RTMPPacket * packet = NULL;//rtmp包结构
	unsigned char * body = NULL;
	//int i;
	packet = (RTMPPacket *)malloc(RTMP_HEAD_SIZE + 1024);
	//RTMPPacket_Reset(packet);//重置packet状态
	memset(packet, 0, RTMP_HEAD_SIZE + 1024);
	packet->m_body = (char *)packet + RTMP_HEAD_SIZE;
	body = (unsigned char *)packet->m_body;



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

	packet->m_body = pbuf;
	packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;
	packet->m_nBodySize = (int)(pbuf - avc_seq_buf);
	packet->m_nChannel = 0x04;
	packet->m_nTimeStamp = 0;
	packet->m_hasAbsTimestamp = 0;
	packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
	packet->m_nInfoField2 = rtmp->m_stream_id;
	int nRet = RTMP_SendPacket(rtmp, packet, TRUE);
	free(packet);    //释放内存
	//Send(avc_seq_buf, (int)(pbuf - avc_seq_buf), 0x09, 0);
}

#include <process.h> /* _beginthread, _endthread */

int main(int argc, char* argv[])
{

	{
		//x264
		x264_encoder_ = new X264Encoder();
		x264_encoder_->Init(1280, 720, 500, 25, 3, 8, 1);
	}





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
	///in_filename  = "rtmp://live.hkstv.hk.lxdns.com/live/hks";

	AVDictionary* ffoptions = NULL;
	av_dict_set(&ffoptions, "rtsp_transport", "tcp", 0);
	av_dict_set(&ffoptions, "fflags", "nobuffer", 0);
	in_filename = "rtsp://192.168.1.175/0/888888:888888/main";

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
	if ((ret = avformat_open_input(&ifmt_ctx, in_filename, 0, &ffoptions)) < 0) {
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

	live_264size_ = 1280 * 720 * 2;
	
	//Write file header
	ret = avformat_write_header(ofmt_ctx, NULL);
	if (ret < 0) {
		printf("Error occurred when opening output URL\n");
		goto end;
	}

	//进入读取程序
	
	_beginthread(beginthread_fun, 0, NULL);
#if USE_H264BSF
	AVBitStreamFilterContext* h264bsfc = av_bitstream_filter_init("h264_mp4toannexb");
#endif
	int num = 0;
	//////////////////////////////////////////////////////////////////////////
#define SAVE_H264_FILE 0
#if SAVE_H264_FILE
	FILE *fpSave;
	if ((fpSave = fopen("mytest333.h264", "ab")) == NULL) //h264保存的文件名  
		return 0;
	unsigned char *dummy = NULL;   //输入的指针  
	int dummy_len;
	AVBitStreamFilterContext* bsfc = av_bitstream_filter_init("h264_mp4toannexb");
	av_bitstream_filter_filter(bsfc, ff_codec_ctx_, NULL, &dummy, &dummy_len, NULL, 0, 0);
	fwrite(ff_codec_ctx_->extradata, ff_codec_ctx_->extradata_size, 1, fpSave);
	av_bitstream_filter_close(bsfc);
	free(dummy);

	for (int i = 0;i < 600;i++)
	{
		//------------------------------  
		if (av_read_frame(ifmt_ctx, &pkt) >= 0)
		{
		
			if ((pkt.stream_index == videoindex))
			{
				
				/*if ((pkt.flags & AV_PKT_FLAG_KEY) && (pkt.size > 0))
				{
				double duration = pkt.duration * 1000.0 / ifmt_ctx->streams[videoindex]->time_base.den;
				}*/
				double duration = pkt.duration * 1000.0 / ifmt_ctx->streams[videoindex]->time_base.den;
				/*	if ((*(pkt.data + 25) == 0x7) || (*(pkt.data + 25) == 0x1))*/

					{
						/*	char nal_start[] = { 0, 0, 0, 1 };
							fwrite(nal_start, 4, 1, fpSave);
							fwrite(pkt.data + 24, pkt.size - 24, 1, fpSave);*/
						fwrite(pkt.data, pkt.size, 1,fpSave);
						fflush(fpSave);
					}
				
					////fwrite(pkt.data, 1, pkt.size, fpSave);//写数据到文件中  
					//		fwrite(pkt.data, 1, pkt.size, fpSave);//写数据到文件中  
				
			}
			av_free_packet(&pkt);
		}
	}
	fclose(fpSave);
#endif
	//////////////////////////////////////////////////////////////////////////
	isKeyframe = true;
	while (1) {
		AVStream *in_stream, *out_stream;
		//Get an AVPacket
		ret = av_read_frame(ifmt_ctx, &pkt);
		if (ret < 0)
			break;
//////////////////////////////////////////////////////////////////////////
		
		 //video_bufs_cache.push_back(&pkt);
		//frame_info(&pkt,videoindex)
		//if (pkt.flags != CODEC_ID_NONE)
		if (pkt.stream_index == videoindex)
		{
			if ((pkt.size > 0) && (pkt.flags & AV_PKT_FLAG_KEY))
			{
				double duration = pkt.duration * 1000.0 / ff_codec_ctx_->time_base.den;

				AVFrame*  avs_frame;
				
				avs_frame = av_frame_alloc();
				
				uint8_t *out_buffer;
				//decode
				int num = 0;
				int av_num = avcodec_decode_video2(ff_codec_ctx_, avs_frame, &num, &pkt);
				if (avs_frame->pict_type != AV_PICTURE_TYPE_NONE)
				{
					AVFrame*  avs_YUVframe;
					avs_YUVframe = av_frame_alloc();
					out_buffer = new uint8_t[avpicture_get_size(AV_PIX_FMT_YUV420P, ff_codec_ctx_->width, ff_codec_ctx_->height)];
					avpicture_fill((AVPicture *)avs_YUVframe, (uint8_t*)out_buffer, AV_PIX_FMT_YUV420P, ff_codec_ctx_->width, ff_codec_ctx_->height);
					struct SwsContext * img_convert_ctx;
					img_convert_ctx = sws_getContext(ff_codec_ctx_->width, ff_codec_ctx_->height, ff_codec_ctx_->pix_fmt, ff_codec_ctx_->width, ff_codec_ctx_->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
					sws_scale(img_convert_ctx, avs_frame->data, avs_frame->linesize, 0, ff_codec_ctx_->height, avs_YUVframe->data, avs_YUVframe->linesize);

					video_frame_list_.push_back(avs_YUVframe);

					//av_frame_free(&avs_YUVframe);
				}
				
				av_frame_free(&avs_frame);
			}
		}
//////////////////////////////////////////////////////////////////////////
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
	//av_bitstream_filter_close(h264bsfc);
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

#define USE_FFMPEG_ENCODEC 1
#if USE_FFMPEG_ENCODEC
	char* pbuf = buf;

	unsigned char flag = 0;
	if (isKeyframe)
		flag = 0x17;
	else
		flag = 0x27;

	pbuf = UI08ToBytes(pbuf, flag);

	pbuf = UI08ToBytes(pbuf, 1);    // avc packet type (0, nalu)
	pbuf = UI24ToBytes(pbuf, 0);    // composition time

#endif
	//timestamp -= i_video_timestamp;
	bool isok = Send(buf, bufLen, 0x09, timestamp);
	//timestamp += 50;
	if (false == isok)
	{
	}
	//i_video_timestamp += 1000;
}


int SendH264Packet(unsigned char *data, unsigned int size, int bIsKeyFrame, unsigned int nTimeStamp);

void frame_info(void/*AVPacket* avpacket,int videoindex*/)
{
	
	while (1)
	{
		//Sleep(6000);
		//continue;
		if (video_frame_list_.empty())
			continue;
		for (std::list<AVFrame *>::iterator it = video_frame_list_.begin(); it != video_frame_list_.end(); ++it)
		{
			//if (it->flags != ) continue;
			
			AVFrame*  avs_frame;
			avs_frame = av_frame_alloc();
			avs_frame = *it;

#define SAVE_PICTURE 0
#if  SAVE_PICTURE
			
			FILE *testfile = fopen("test5.yuv", "wb");
			fwrite(avs_frame->data[0], ff_codec_ctx_->width*ff_codec_ctx_->height, 1, testfile);
			fwrite(avs_frame->data[1], ff_codec_ctx_->width*ff_codec_ctx_->height / 4, 1, testfile);
			fwrite(avs_frame->data[2], ff_codec_ctx_->width*ff_codec_ctx_->height / 4, 1, testfile);
			//fwrite(&nalbuf[0], ff_codec_ctx_->width*ff_codec_ctx_->height, 1, testfile);
			//fwrite(&nalbuf[1], ff_codec_ctx_->width*ff_codec_ctx_->height / 4, 1, testfile);
			//fwrite(&nalbuf[2], ff_codec_ctx_->width*ff_codec_ctx_->height / 4, 1, testfile);
			fflush(testfile);
			fclose(testfile);
#endif
#define USE_LIST_ 0
#if USE_LIST_
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

				int a = avpacket->buf->size;

				if (!(avs_frame->pict_type == AV_PICTURE_TYPE_NONE))
				{

#endif
					//SendAVCSequenceHeaderPacket();
					if (avs_frame->pict_type == AV_PKT_FLAG_KEY)
					{
						//SendAVCSequenceHeaderPacket();
						isKeyframe = true;

					}
					//uint8_t *buf = (uint8_t *)malloc(*avs_frame->pkt_size);
					//avcodec_encode_video(ff_encodec_ctx_, buf, *avs_frame->linesize, avs_frame);


					//int pktsize = avpacket->size;
					int keyframe = avs_frame->key_frame;
					int width = avs_frame->width;
					int hight = avs_frame->height;
					printf("keyframe = %d \ n", keyframe);
					printf("width = %d \n", width);
					printf("hight = %d \n", hight);
					//printf("pktsize = %d \n", pktsize);

					{
						//int num = 1;
						//FILE *testfile = fopen("test2.rgb", "wb");
						//////////////////////////////////////////////////////////////////////////
#if USE_LIST_					
						out_buffer = new uint8_t[avpicture_get_size(AV_PIX_FMT_YUV420P, ff_codec_ctx_->width, ff_codec_ctx_->height)];
						avpicture_fill((AVPicture *)avs_YUVframe, (uint8_t*)out_buffer, AV_PIX_FMT_YUV420P, ff_codec_ctx_->width, ff_codec_ctx_->height);
						struct SwsContext * img_convert_ctx;
						img_convert_ctx = sws_getContext(ff_codec_ctx_->width, ff_codec_ctx_->height, ff_codec_ctx_->pix_fmt, ff_codec_ctx_->width, ff_codec_ctx_->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
						sws_scale(img_convert_ctx, avs_frame->data, avs_frame->linesize, 0, ff_codec_ctx_->height, avs_YUVframe->data, avs_YUVframe->linesize);
						//RGB  
						//fwrite(avs_YUVframe->data[0], (ff_codec_ctx_->width)*(ff_codec_ctx_->height) * 3, 1, testfile);
#endif
						//////////////////////////////////////////////////////////////////////////
#define X264_MODE 1
#if X264_MODE
						
						
					/*
					        live_264buf_ = new char[live_264size_];
						    avpicture_fill((AVPicture *)live_264buf_, (uint8_t*)avs_frame,
							PIX_FMT_YUV420P, 1280, 720);
							SwsContext *sws_ctx_ = sws_getContext(1920, 1080, AV_PIX_FMT_YUV420P,
							1280, 720, PIX_FMT_RGB32,
							SWS_BICUBIC, 0, 0, 0);
							sws_scale(sws_ctx_, avs_frame->data, avs_frame->linesize, 0,
							height_, live_264buf_, live_264size_);*/
						//AVFrame *src_frame_= av_frame_alloc();
						AVFrame *yuv_frame_ = av_frame_alloc();
				
						int live_yuvsize_ = 1280 * 720 * 3 / 2;
						char* live_yuvbuf_ = new char[live_yuvsize_];

						avpicture_fill((AVPicture *)yuv_frame_, (uint8_t*)live_yuvbuf_,
							PIX_FMT_YUV420P, 1280, 720);

						SwsContext *sws_ctx_ = sws_getContext(1920, 1080, AV_PIX_FMT_YUV420P,
							1280, 720, AV_PIX_FMT_YUV420P,
							SWS_BICUBIC, 0, 0, 0);
						sws_scale(sws_ctx_, avs_frame->data, avs_frame->linesize, 0,
							720, yuv_frame_->data, yuv_frame_->linesize);



						int outlen = live_264size_;

						char* nalbuf = x264_encoder_->Encode((unsigned char*)live_yuvbuf_,
							(unsigned char*)live_264buf_, outlen, isKeyframe);

						
#else			
						AVPacket av_pakt;
						av_init_packet(&av_pakt);
						av_pakt.size = avpicture_get_size(ff_encodec_ctx_->pix_fmt, ff_encodec_ctx_->width, ff_encodec_ctx_->height);
						av_pakt.data = NULL;
						int got_packet_ptr = 0;
						int anv = avcodec_encode_video2(ff_encodec_ctx_, &av_pakt, avs_frame, &got_packet_ptr);

#endif
						//////////////////////////////////////////////////////////////////////////
						//////////////////////////////////////////////////////////////////////////
						/*AVFrame  *testframe = NULL;
						testframe = av_frame_alloc();
						avcodec_decode_video2(ff_codec_ctx_, testframe, &num, &av_pakt);*/
						//FILE *testfile = fopen("test4.yuv", "wb");
						/*fwrite(avs_YUVframe->data[0], ff_codec_ctx_->width*ff_codec_ctx_->height, 1, testfile);
						fwrite(avs_YUVframe->data[1], ff_codec_ctx_->width*ff_codec_ctx_->height / 4, 1, testfile);
						fwrite(avs_YUVframe->data[2], ff_codec_ctx_->width*ff_codec_ctx_->height / 4, 1, testfile);*/
						/*fwrite(&nalbuf[0], ff_codec_ctx_->width*ff_codec_ctx_->height, 1, testfile);
						fwrite(&nalbuf[1], ff_codec_ctx_->width*ff_codec_ctx_->height / 4, 1, testfile);
						fwrite(&nalbuf[2], ff_codec_ctx_->width*ff_codec_ctx_->height / 4, 1, testfile);
						fflush(testfile);
						fclose(testfile);
						*/
#if X264_MODE
						if (nalbuf)
							//SendVideoData(nalbuf, outlen, timestamp, isKeyframe);
							SendH264Packet((unsigned char*)nalbuf, outlen, isKeyframe, timestamp);
						//isKeyframe = false;
						Sleep(100);
						delete live_yuvbuf_;
						
						//free(nalbuf);
						live_yuvbuf_ = NULL;
						//video_frame_list_.pop_front();
						av_frame_free(&yuv_frame_);
						//av_frame_free(&avs_frame);
						avs_frame = NULL;
						
						
						//nalbuf = NULL;

#else									
						if (av_pakt.data)
						{
							double duration = av_pakt.duration * 1000.0 / ff_encodec_ctx_->time_base.den;
							//ifmt_ctx->streams[videoindex]->time_base.den;
							SendVideoData((char*)av_pakt.data, av_pakt.size, timestamp, isKeyframe);
							Sleep(100);
						}
						isKeyframe = false;
						av_frame_free(&avs_frame);


#endif
						//Sleep(200);
						isKeyframe = false;
						timestamp += 40;
						if (video_frame_list_.empty())
							return;
						//delete out_buffer;
						//out_buffer = NULL;
						//delete x264_encoder_;
						//return;
						/*fwrite(avs_YUVframe->data[0], ff_codec_ctx_->width*ff_codec_ctx_->height, 1, testfile);
						fwrite(avs_YUVframe->data[1], ff_codec_ctx_->width*ff_codec_ctx_->height / 4, 1, testfile);
						fwrite(avs_YUVframe->data[2], ff_codec_ctx_->width*ff_codec_ctx_->height / 4, 1, testfile);*/
						//fflush(testfile);
						//fclose(testfile);
					
					}
					/*	}*/

					//char *video_frame = new char(avpacket->size);
					//video_bufs_cache.push_back(video_frame);
					//av_frame_free(&avs_frame);
					//av_frame_free(&avs_YUVframe);
				}

		}
}











////////////////////////////////////////




int SendVideoSpsPps(unsigned char *pps, int pps_len, unsigned char * sps, int sps_len);
int SendPacket(unsigned int nPacketType, unsigned char *data, unsigned int size, unsigned int nTimestamp);

int SendH264Packet(unsigned char *data, unsigned int size, int bIsKeyFrame, unsigned int nTimeStamp)
{
	if (data == NULL && size < 11){
		return false;
	}

	unsigned char *body = (unsigned char*)malloc(size + 9);
	memset(body, 0, size + 9);

	int i = 0;
	if (bIsKeyFrame){
		body[i++] = 0x17;// 1:Iframe  7:AVC   
		body[i++] = 0x01;// AVC NALU   
		body[i++] = 0x00;
		body[i++] = 0x00;
		body[i++] = 0x00;


		// NALU size   
		body[i++] = size >> 24 & 0xff;
		body[i++] = size >> 16 & 0xff;
		body[i++] = size >> 8 & 0xff;
		body[i++] = size & 0xff;
		// NALU data   
		memcpy(&body[i], data, size);
		SendVideoSpsPps((unsigned char*)pps_, pps_size_, (unsigned char*)sps_, sps_size_);
	}
	else{
		body[i++] = 0x27;// 2:Pframe  7:AVC   
		body[i++] = 0x01;// AVC NALU   
		body[i++] = 0x00;
		body[i++] = 0x00;
		body[i++] = 0x00;


		// NALU size   
		body[i++] = size >> 24 & 0xff;
		body[i++] = size >> 16 & 0xff;
		body[i++] = size >> 8 & 0xff;
		body[i++] = size & 0xff;
		// NALU data   
		memcpy(&body[i], data, size);
	}


	int bRet = SendPacket(RTMP_PACKET_TYPE_VIDEO, body, i + size, nTimeStamp);

	free(body);

	return bRet;
}


int SendVideoSpsPps(unsigned char *pps, int pps_len, unsigned char * sps, int sps_len)
{
	RTMPPacket * packet = NULL;//rtmp包结构
	unsigned char * body = NULL;
	int i;
	packet = (RTMPPacket *)malloc(RTMP_HEAD_SIZE + 1024);
	//RTMPPacket_Reset(packet);//重置packet状态
	memset(packet, 0, RTMP_HEAD_SIZE + 1024);
	packet->m_body = (char *)packet + RTMP_HEAD_SIZE;
	body = (unsigned char *)packet->m_body;
	i = 0;
	body[i++] = 0x17;
	body[i++] = 0x00;

	body[i++] = 0x00;
	body[i++] = 0x00;
	body[i++] = 0x00;

	/*AVCDecoderConfigurationRecord*/
	body[i++] = 0x01;
	body[i++] = sps[1];
	body[i++] = sps[2];
	body[i++] = sps[3];
	body[i++] = 0xff;

	/*sps*/
	body[i++] = 0xe1;
	body[i++] = (sps_len >> 8) & 0xff;
	body[i++] = sps_len & 0xff;
	memcpy(&body[i], sps, sps_len);
	i += sps_len;

	/*pps*/
	body[i++] = 0x01;
	body[i++] = (pps_len >> 8) & 0xff;
	body[i++] = (pps_len)& 0xff;
	memcpy(&body[i], pps, pps_len);
	i += pps_len;

	packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;
	packet->m_nBodySize = i;
	packet->m_nChannel = 0x04;
	packet->m_nTimeStamp = 0;
	packet->m_hasAbsTimestamp = 0;
	packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
	packet->m_nInfoField2 = rtmp->m_stream_id;

	/*调用发送接口*/
	int nRet = RTMP_SendPacket(rtmp, packet, TRUE);
	free(packet);    //释放内存
	return nRet;
}
int SendPacket(unsigned int nPacketType, unsigned char *data, unsigned int size, unsigned int nTimestamp)
{
	RTMPPacket* packet;
	/*分配包内存和初始化,len为包体长度*/
	packet = (RTMPPacket *)malloc(RTMP_HEAD_SIZE + size);
	memset(packet, 0, RTMP_HEAD_SIZE);
	/*包体内存*/
	packet->m_body = (char *)packet + RTMP_HEAD_SIZE;
	packet->m_nBodySize = size;
	memcpy(packet->m_body, data, size);
	packet->m_hasAbsTimestamp = 0;
	packet->m_packetType = nPacketType; /*此处为类型有两种一种是音频,一种是视频*/
	packet->m_nInfoField2 = rtmp->m_stream_id;
	packet->m_nChannel = 0x04;

	packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
	if (RTMP_PACKET_TYPE_AUDIO == nPacketType && size != 4)
	{
		packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
	}
	packet->m_nTimeStamp = nTimestamp;
	/*发送*/
	int nRet = 0;
	if (RTMP_IsConnected(rtmp))
	{
		nRet = RTMP_SendPacket(rtmp, packet, TRUE); /*TRUE为放进发送队列,FALSE是不放进发送队列,直接发送*/
	}
	/*释放内存*/
	free(packet);
	return nRet;
}

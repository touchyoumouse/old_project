/*******************************************************************************
* x264_encoder.cpp
* Copyright: (c) 2014 Haibin Du(haibindev.cnblogs.com). All rights reserved.
* -----------------------------------------------------------------------------
*
* -----------------------------------------------------------------------------
* 2015-6-4 19:47 - Created (Haibin Du)
******************************************************************************/

#include "encodec.h"
#include "string.h"
#include "stdio.h"

//#include "base/platform.h"
//#include "base/byte_stream.h"
//#include "base/simple_logger.h"

//#if defined(HB_PLATFORM_WIN32)
#if defined(HDEV_USE_X264DLL)
//#pragma comment(lib, "base_encoder/x264/libx264-148.lib")
#else
#if defined(_DEBUG)
#pragma comment(lib, "./base_encoder/libx264/libx264d.lib")
#else
#pragma comment(lib, "./base_encoder/libx264/libx264.lib")
#endif
#endif
//#endif

X264Encoder::X264Encoder()
{
	x264_handle_ = NULL;
	x264_param_default(&param_);
}

X264Encoder::~X264Encoder()
{
	Destroy();
}

int X264Encoder::Init(int width, int height, int bitrate,
	int fps, int quality, int keyInterSecs, bool useCBR)
{
#if defined(HDEV_USE_X264DLL)
	//x264_param_default_preset(&param_, "veryfast", "zerolatency");
	x264_param_default_preset(&param_, "superfast", "zerolatency");
	//x264_param_default_preset(&param_, "ultrafast", "zerolatency");
#else
	param_.i_bframe = 0;                   // no bframes
	param_.i_scenecut_threshold = 0;      // scenecut = -1, or 0?
	param_.b_cabac = 0;
	param_.analyse.b_transform_8x8 = 0;    // no-8x8dct
	param_.analyse.i_me_method = 0;        // me = dia
	param_.analyse.i_subpel_refine = 0;    // subme = 0, or 1?
	param_.analyse.inter = 0;              // partitions = none
#endif

	param_.i_bframe = 0;
	param_.i_width = width;
	param_.i_height = height;

	param_.i_fps_num = fps;
	param_.i_fps_den = 1;

	if (useCBR)
	{
		param_.rc.i_rc_method = X264_RC_ABR;
		param_.rc.f_rf_constant = 0.0f;
	}
	else
	{
		param_.rc.i_rc_method = X264_RC_CRF;
		param_.rc.f_rf_constant = 22.0 + float(10 - quality);
	}

	param_.rc.i_bitrate = bitrate;
	param_.rc.i_vbv_max_bitrate = bitrate*1.2;

	//param_.i_threads = 4;

	if (keyInterSecs > 0)
	{
		param_.i_keyint_max = keyInterSecs*fps;
		param_.i_keyint_min = keyInterSecs*fps;
	}
	else
	{
		param_.i_keyint_max = 7 * fps;
		param_.i_keyint_min = 4 * fps;
	}
	param_.i_frame_total = 0;

	param_.i_log_level = X264_LOG_ERROR;

#if defined(HDEV_USE_X264DLL)
	param_.i_level_idc = 31;
	x264_param_apply_profile(&param_, "baseline");
#endif

	//SIMPLE_LOG("useCBR: %d\n", useCBR);

	/* 根据输入参数param初始化总结构 x264_t *h     */
	if ((x264_handle_ = x264_encoder_open(&param_)) == NULL)
	{
		//fprintf( stderr, "x264 [error]: x264_encoder_open failed\n" );

		return -1;
	}

	x264_picture_alloc(&picture_, X264_CSP_I420, param_.i_width, param_.i_height);
	picture_.i_type = X264_TYPE_AUTO;
	picture_.i_qpplus1 = 0;

	return 0;
}

int X264Encoder::Destroy()
{
	if (x264_handle_)
	{
		//x264_picture_clean( &m_pic );

		x264_encoder_close(x264_handle_);
	}

	return 0;
}

char* X264Encoder::Encode(unsigned char* szYUVFrame, unsigned char* outBuf,
	int& outLen, bool& isKeyframe)
{

	char* ret_264buf = NULL;

	picture_.img.plane[0] = szYUVFrame;
	picture_.img.plane[1] = szYUVFrame + param_.i_width*param_.i_height;
	picture_.img.plane[2] = picture_.img.plane[1] + param_.i_width*param_.i_height / 4;

	param_.i_frame_total++;
	picture_.i_pts = (int64_t)param_.i_frame_total * param_.i_fps_den;
	if (isKeyframe)
		picture_.i_type = X264_TYPE_IDR;
	else
		picture_.i_type = X264_TYPE_AUTO;

	x264_picture_t pic_out;
	x264_nal_t *nal = 0;
	int i_nal, i; // nal的个数

#if !defined(HDEV_USE_X264DLL)
	if (x264_encoder_encode(x264_handle_, &nal, &i_nal, &picture_, &pic_out) < 0)
	{
		//fprintf( stderr, "x264 [error]: x264_encoder_encode failed\n" );
		return ret_264buf;
	}

	int maxlen = outLen;
	outLen = 0;
	for (i = 0; i < i_nal; i++)
	{
		int i_size = 0;
		x264_nal_encode(outBuf + outLen, &i_size, 1, &nal[i]);

		// 将起始码0x00000001，替换为nalu的大小
		//UI32ToBytes((char*)(outBuf+outLen), i_size-4);
		outLen += i_size;
	}

	ret_264buf = (char*)outBuf;

#else
	int i_frame_size = x264_encoder_encode(x264_handle_, &nal, &i_nal, &picture_, &pic_out);
	if (i_frame_size < 0)
	{
		return ret_264buf;
	}

	if (i_frame_size)
	{
		//memcpy(outBuf, nal[0].p_payload, i_frame_size);
		ret_264buf = (char*)nal[0].p_payload;
		outLen = i_frame_size;
	}
#endif

	isKeyframe = (pic_out.i_type == X264_TYPE_IDR);

	return ret_264buf;
}

void X264Encoder::GetExtraCodecInfo(unsigned char* extraData, int* extraSize)
{
	x264_nal_t *nal;
	int nnal;
	int s = x264_encoder_headers(x264_handle_, &nal, &nnal);

	unsigned char* pdata = extraData;
	for (int i = 0; i < nnal; i++)
	{
		/* Don't put the SEI in extradata. */
		if (nal[i].i_type == NAL_SEI)
		{
			continue;
		}
		memcpy(pdata, nal[i].p_payload, nal[i].i_payload);
		pdata += nal[i].i_payload;
	}

	*extraSize = pdata - extraData;
}


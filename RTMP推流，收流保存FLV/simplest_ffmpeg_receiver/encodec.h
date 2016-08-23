/*******************************************************************************
* x264_encoder.h
* Copyright: (c) 2014 Haibin Du(haibindev.cnblogs.com). All rights reserved.
* -----------------------------------------------------------------------------
*
* h.264编码器，调用x264实现
*
* -----------------------------------------------------------------------------
* 2015-6-4 19:47 - Created (Haibin Du)
******************************************************************************/

#ifndef _HDEV_X264_ENCODER_H_
#define _HDEV_X264_ENCODER_H_

//#include "base/base.h"

#include "stdint.h"
//#include <cstdio>

#define HDEV_USE_X264DLL 1

extern "C"
{
#if defined(HDEV_USE_X264DLL)
#include "base_encoder/x264/x264.h"
#else
#include "libx264/x264.h"
#endif
}

struct TNAL
{
	int size;
	unsigned char* data;
	TNAL() : size(0), data(NULL) {}
};

class X264Encoder
{
public:
	X264Encoder();

	~X264Encoder();

	/***
	* 初始化编码器
	* @param width: 宽度
	* @param height: 高度
	* @param bitrate: 视频码率
	* @param fps: 视频帧率
	* @param keyInterSec: 关键帧间隔
	* @returns:
	*/
	int Init(int width, int height, int bitrate, int fps, int quality,
		int keyInterSecs = 0, bool useCBR = false);

	/***
	* 编码
	* @param yuvBuf: 输入YUV420P数据
	* @param outBuf: 输出h.264数据地址
	* @param outLen: 输出大小
	* @param isKeyframe: 是否是关键帧
	* @returns:
	*/
	char* Encode(unsigned char* yuvBuf, unsigned char* outBuf, int& outLen,
		bool& isKeyframe);

	int Destroy();

	/***
	* 获取编码参数，包含sps和pps数据
	* @param extraData: 输出数据地址
	* @param extraSize: 输出大小
	* @returns:
	*/
	void GetExtraCodecInfo(unsigned char* extraData, int* extraSize);

private:
	x264_param_t param_;
	x264_picture_t picture_;
	x264_t* x264_handle_;
};

#endif // _HDEV_X264_ENCODER_H_

/*******************************************************************************
* x264_encoder.h
* Copyright: (c) 2014 Haibin Du(haibindev.cnblogs.com). All rights reserved.
* -----------------------------------------------------------------------------
*
* h.264������������x264ʵ��
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
	* ��ʼ��������
	* @param width: ���
	* @param height: �߶�
	* @param bitrate: ��Ƶ����
	* @param fps: ��Ƶ֡��
	* @param keyInterSec: �ؼ�֡���
	* @returns:
	*/
	int Init(int width, int height, int bitrate, int fps, int quality,
		int keyInterSecs = 0, bool useCBR = false);

	/***
	* ����
	* @param yuvBuf: ����YUV420P����
	* @param outBuf: ���h.264���ݵ�ַ
	* @param outLen: �����С
	* @param isKeyframe: �Ƿ��ǹؼ�֡
	* @returns:
	*/
	char* Encode(unsigned char* yuvBuf, unsigned char* outBuf, int& outLen,
		bool& isKeyframe);

	int Destroy();

	/***
	* ��ȡ�������������sps��pps����
	* @param extraData: ������ݵ�ַ
	* @param extraSize: �����С
	* @returns:
	*/
	void GetExtraCodecInfo(unsigned char* extraData, int* extraSize);

private:
	x264_param_t param_;
	x264_picture_t picture_;
	x264_t* x264_handle_;
};

#endif // _HDEV_X264_ENCODER_H_

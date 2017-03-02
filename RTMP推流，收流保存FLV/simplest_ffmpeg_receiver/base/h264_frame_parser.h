/*******************************************************************************
 * h264_frame_parser.h
 * Copyright: (c) 2014 Haibin Du(haibindev.cnblogs.com). All rights reserved.
 * -----------------------------------------------------------------------------
 *
 *
 *
 * -----------------------------------------------------------------------------
 * 2015-6-5 14:33 - Created (Haibin Du)
 ******************************************************************************/

#ifndef _HDEV_H264_FRAME_PARSER_H_
#define _HDEV_H264_FRAME_PARSER_H_

const char* AVCFindStartCodeInternal(const char *p, const char *end);

const char* AVCFindStartCode(const char *p, const char *end);

void AVCParseNalUnits(const char *bufIn, int inSize, char* bufOut, int* outSize);

void ParseH264Frame(const char* nalsbuf, int size, char* outBuf, int& outLen,
    char* spsBuf, int& spsSize, char* ppsBuf, int& ppsSize,
    bool& isKeyframe, int* pwidth, int* pheight);

bool CheckH264Frame(const char* frameBuf, int bufLen,
    char* spsBuf, int* spsSize, char* ppsBuf, int* ppsSize,
    bool& isKeyframe);

int ff_h264_decode_sps(const char* spsBuf, int spsLen,
    int* pwidth, int* pheight);

bool GetWidthHeightFromFrame(const char* frameBuf, int bufLen, int& width, int& height,
    char* spsBuf, int& spsSize, char* ppsBuf, int& ppsSize);

#endif // _HDEV_H264_FRAME_PARSER_H_

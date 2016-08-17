#include "stdafx.h"
#include "RtmpLiveScreen.h"

#include <vector>

#include "dshow/DSVideoGraph.h"
#include "AmfByteStream.h"
#include "BitWritter.h"
#include "LibRtmp.h"

#define ENCODE_AUDIO_USE_MP3

RtmpLiveScreen::RtmpLiveScreen(const CString& audioDeviceID, const CString& videoDeviceID,
    int width, int height, OAHWND owner, int previewWidth, int previewHeight,
    const std::string& rtmpUrl, bool isNeedScreen, bool isNeedRecord,
    int capX, int capY, int capWidth, int capHeight,
    int webcamBitrate, int screenBitrate, int videoChangeMillsecs,int waitTime)
{
    ds_capture_ = new DSCapture();

    DSAudioFormat audio_fmt;
    audio_fmt.samples_per_sec = 22050;
    audio_fmt.channels = 1;
    audio_fmt.bits_per_sample = 16;

    DSVideoFormat video_fmt;
    video_fmt.width = capWidth;
    video_fmt.height = capHeight;
    video_fmt.fps = 15;

    if (audioDeviceID.IsEmpty())
        has_audio_ = false;
    else
        has_audio_ = true;

    ds_capture_->Create(audioDeviceID, videoDeviceID, audio_fmt, video_fmt, _T(""), _T("./test.264"),
        capX, capY, capWidth, capHeight, isNeedScreen, webcamBitrate, screenBitrate);
    //ds_capture_->AdjustVideoWindow(owner, previewWidth, previewHeight);
    ds_capture_->SetListener(this);

    width_ = ds_capture_->GetVideoGraph()->Width();
    height_ = ds_capture_->GetVideoGraph()->Height();

    sps_ = NULL;
    sps_size_ = 0;
    pps_ = NULL;
    pps_size_ = 0;

    rtmp_url_ = rtmpUrl;
    librtmp_ = new LibRtmp(false, isNeedRecord);

    video_change_millsecs_ = videoChangeMillsecs;
    state_ = STATE_IDLE;

    last_input_timestamp_ = GetInputTimestamp();

    ppt_fetcher_ = new CpCount;
    ppt_fetcher_->CreateDispatch(_T("PPTCount.pCount"));

    last_page_count_ = -1;
	//@feng liveScreen wait time;
	wait_time_ = waitTime;

    is_send_failed_ = false;


}

RtmpLiveScreen::~RtmpLiveScreen()
{
    if (ppt_fetcher_)
    {
        ppt_fetcher_->ReleaseDispatch();
        delete ppt_fetcher_;
    }

    if (ds_capture_)
        delete ds_capture_;

    if (librtmp_)
        delete librtmp_;

    if (sps_)
        free(sps_);
    if (pps_)
        free(pps_);
}

void RtmpLiveScreen::SetCaptureWebcam(bool isCapture)
{
    if (ds_capture_)
    {
        ds_capture_->SetCaptureWebcam(isCapture);
    }
}

void RtmpLiveScreen::SetWebcamPosition(int x, int y)
{
    if (ds_capture_)
    {
        ds_capture_->SetWebcamPosition(x, y);
    }
}

void RtmpLiveScreen::Run()
{
rtmp_connect_loop:
	
	//@feng Delay to connect;
	Sleep(wait_time_*60*1000);

    // 连接rtmp server，完成握手等协议
    if(!librtmp_->Open(rtmp_url_.c_str()))
	{
		//MessageBox(NULL,_T("连接流媒体服务器失败!"),_T("提示"),MB_OK);
		librtmp_->control_rtmp_=true;
		goto rtmp_connect_loop;
	}

    // 发送metadata包
    SendMetadataPacket();

    librtmp_->CreateSharedObject();

    // 开始捕获音视频
    ds_capture_->StartAudio();
    ds_capture_->StartVideo();

    while (true)
    {
		
        if (SimpleThread::IsStop()) 
				goto rtmp_end;/*break*/;

        // 从队列中取出音频或视频数据
        std::deque<RtmpDataBuffer> rtmp_datas;
        {
            base::AutoLock alock(queue_lock_);
            if (false == process_buf_queue_.empty())
            {
                rtmp_datas = process_buf_queue_;
                process_buf_queue_.clear();
//                 rtmp_data = process_buf_queue_.front();
//                 process_buf_queue_.pop_front();
            }
        }
        // 加上时戳，发送给rtmp server
        for (std::deque<RtmpDataBuffer>::iterator it = rtmp_datas.begin();
            it != rtmp_datas.end(); ++it)
        {
			/*RtmpDataBuffer& rtmp_data = *it;
			if (rtmp_data.data != NULL)
			{
				if (rtmp_data.type == FLV_TAG_TYPE_AUDIO)
					if(SendAudioDataPacket(rtmp_data.data, rtmp_data.timestamp)) break;
				else
					if(SendVideoDataPacket(rtmp_data.data, rtmp_data.timestamp, rtmp_data.is_keyframe))	break;
			}*/
			
        }

        Sleep(1);
    }

    ds_capture_->StopVideo();
    ds_capture_->StopAudio();

    librtmp_->Close();

	goto rtmp_connect_loop;

rtmp_end: 

	ds_capture_->StopVideo();
	ds_capture_->StopAudio();

	librtmp_->Close();
	return ;
}

void RtmpLiveScreen::OnCaptureAudioBuffer(base::DataBuffer* dataBuf, unsigned int timestamp)
{
    if (SimpleThread::IsStop()) return;
    {
        base::AutoLock alock(queue_lock_);
        //process_buf_queue_.push_back(RtmpDataBuffer(FLV_TAG_TYPE_AUDIO, dataBuf, GetTimestamp(), false));

        SendAudioDataPacket(dataBuf, GetTimestamp());
    }
}

void RtmpLiveScreen::OnCaptureVideoBuffer(base::DataBuffer* dataBuf, unsigned int timestamp, bool isKeyframe)
{
    if (SimpleThread::IsStop()) return;
    {
        base::AutoLock alock(queue_lock_);
        //process_buf_queue_.push_back(RtmpDataBuffer(FLV_TAG_TYPE_VIDEO, dataBuf->Clone(), GetTimestamp(), isKeyframe));

        SendVideoDataPacket(dataBuf, GetTimestamp(), isKeyframe);
    }

    // 发送共享对象-当前ppt页码
    int page_count = GetCurrentPPTPCount();
    if (page_count != last_page_count_)
    {
        librtmp_->SendSharedObject(page_count);
        last_page_count_ = page_count;
    }

#if 0
    unsigned int input_ts = GetInputTimestamp();
    if (last_input_timestamp_ != input_ts)
    {
        if (state_ != STATE_WORKING)
        {
            librtmp_->SetSharedObject(true);
            state_ = STATE_WORKING;
        }
        
        last_input_timestamp_ = input_ts;
    }
    else
    {
        if (state_ == STATE_WORKING)
        {
            unsigned int now = ::GetTickCount();
            if (now > last_input_timestamp_)
            {
                if (now >= (last_input_timestamp_+video_change_millsecs_))
                {
                    librtmp_->SetSharedObject(false);
                    state_ = STATE_IDLE;
                }
            }
            else
            {
                if (now > video_change_millsecs_)
                {
                    librtmp_->SetSharedObject(false);
                    state_ = STATE_IDLE;
                }
            }
        }
    }
#endif
}

int RtmpLiveScreen::GetCurrentPPTPCount()
{
    CString page_count_str;
    {
        CpCount	*oo = new CpCount;
        oo->CreateDispatch(_T("PPTCount.pCount"));
        page_count_str = oo->GetPptCount(_T("任意字符串"));
        oo->ReleaseDispatch();
        delete oo;
    }

    if (ppt_fetcher_)
    {
        //page_count_str = ppt_fetcher_->GetPptCount(_T("任意字符串"));
        int pos = page_count_str.Find(',');
        if (pos > 0)
        {
            int total_count = _ttoi(page_count_str.Mid(0, pos));
            if (total_count > 0)
            {
                page_count_str = page_count_str.Mid(pos+1);
                return _ttoi(page_count_str);
            }
        }
    }
    return -1;
}

bool RtmpLiveScreen::SendVideoDataPacket(base::DataBuffer* dataBuf, unsigned int timestamp, bool isKeyframe)
{
    char* buf = (char*)malloc(dataBuf->BufLen() + 5);
    char* pbuf = buf;

    unsigned char flag = 0;
    if (isKeyframe)
        flag = 0x17;
    else
        flag = 0x27;

    pbuf = UI08ToBytes(pbuf, flag);

    pbuf = UI08ToBytes(pbuf, 1);    // avc packet type (0, nalu)
    pbuf = UI24ToBytes(pbuf, 0);    // composition time

    memcpy(pbuf, dataBuf->Buf(), dataBuf->BufLen());
    pbuf += dataBuf->BufLen();

    bool is_ok = librtmp_->Send(buf, (int)(pbuf - buf), FLV_TAG_TYPE_VIDEO, timestamp);
    is_send_failed_ = !is_ok;
	free(buf);
	delete dataBuf;

	return is_ok ? true : false;
}

bool RtmpLiveScreen::SendAudioDataPacket(base::DataBuffer* dataBuf, unsigned int timestamp)
{
    char* buf = (char*)malloc(dataBuf->BufLen() + 5);
    char* pbuf = buf;

#ifdef ENCODE_AUDIO_USE_MP3
    unsigned char flag = 0;
    flag = (2 << 4) |   // soundformat "2  == MP3"
        (2 << 2) |      // soundrate   "2  == 22-kHZ"
        (1 << 1) |      // soundsize   "1  == 16bit"
        1;              // soundtype   "1  == Stereo"

    pbuf = UI08ToBytes(pbuf, flag);

#else
    unsigned char flag = 0;
    flag = (10 << 4) |  // soundformat "10 == AAC"
        (3 << 2) |      // soundrate   "3  == 44-kHZ"
        (1 << 1) |      // soundsize   "1  == 16bit"
        1;              // soundtype   "1  == Stereo"

    pbuf = UI08ToBytes(pbuf, flag);

    pbuf = UI08ToBytes(pbuf, 1);    // aac packet type (1, raw)

#endif

    memcpy(pbuf, dataBuf->Buf(), dataBuf->BufLen());
    pbuf += dataBuf->BufLen();

    if(librtmp_->Send(buf, (int)(pbuf - buf), FLV_TAG_TYPE_AUDIO, timestamp))
	{
		free(buf);
		delete dataBuf;
		return true;
	}
	else
	{
		free(buf);
		delete dataBuf;
		return false;
	}
}

void RtmpLiveScreen::PostBuffer(base::DataBuffer* dataBuf)
{

}



void RtmpLiveScreen::SendMetadataPacket()
{
    char metadata_buf[4096];
    char* pbuf = WriteMetadata(metadata_buf);

    librtmp_->Send(metadata_buf, (int)(pbuf - metadata_buf), FLV_TAG_TYPE_META, 0);
}

char* RtmpLiveScreen::WriteMetadata(char* buf)
{
    char* pbuf = buf;

    pbuf = UI08ToBytes(pbuf, AMF_DATA_TYPE_STRING);
    pbuf = AmfStringToBytes(pbuf, "@setDataFrame");

    pbuf = UI08ToBytes(pbuf, AMF_DATA_TYPE_STRING);
    pbuf = AmfStringToBytes(pbuf, "onMetaData");

//     pbuf = UI08ToBytes(pbuf, AMF_DATA_TYPE_MIXEDARRAY);
//     pbuf = UI32ToBytes(pbuf, 2);
    pbuf = UI08ToBytes(pbuf, AMF_DATA_TYPE_OBJECT);

    pbuf = AmfStringToBytes(pbuf, "width");
    pbuf = AmfDoubleToBytes(pbuf, width_);

    pbuf = AmfStringToBytes(pbuf, "height");
    pbuf = AmfDoubleToBytes(pbuf, height_);

    pbuf = AmfStringToBytes(pbuf, "framerate");
    pbuf = AmfDoubleToBytes(pbuf, 10);

    pbuf = AmfStringToBytes(pbuf, "videocodecid");
    pbuf = UI08ToBytes(pbuf, AMF_DATA_TYPE_STRING);
    pbuf = AmfStringToBytes(pbuf, "avc1");

    // 0x00 0x00 0x09
    pbuf = AmfStringToBytes(pbuf, "");
    pbuf = UI08ToBytes(pbuf, AMF_DATA_TYPE_OBJECT_END);
    
    return pbuf;
}

void RtmpLiveScreen::SendAVCSequenceHeaderPacket()
{
    char avc_seq_buf[4096];
    char* pbuf = WriteAVCSequenceHeader(avc_seq_buf);

    librtmp_->Send(avc_seq_buf, (int)(pbuf - avc_seq_buf), FLV_TAG_TYPE_VIDEO, 0);
}

char* RtmpLiveScreen::WriteAVCSequenceHeader(char* buf)
{
    char* pbuf = buf;

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

    return pbuf;
}

void RtmpLiveScreen::SendAACSequenceHeaderPacket()
{
    char aac_seq_buf[4096];
    char* pbuf = WriteAACSequenceHeader(aac_seq_buf);

    librtmp_->Send(aac_seq_buf, (int)(pbuf - aac_seq_buf), FLV_TAG_TYPE_AUDIO, 0);
}

char* RtmpLiveScreen::WriteAACSequenceHeader(char* buf)
{
    char* pbuf = buf;

    unsigned char flag = 0;
    flag = (10 << 4) |  // soundformat "10 == AAC"
        (3 << 2) |      // soundrate   "3  == 44-kHZ"
        (1 << 1) |      // soundsize   "1  == 16bit"
        1;              // soundtype   "1  == Stereo"

    pbuf = UI08ToBytes(pbuf, flag);

    pbuf = UI08ToBytes(pbuf, 0);    // aac packet type (0, header)

    // AudioSpecificConfig

    PutBitContext pb;
    init_put_bits(&pb, pbuf, 1024);
    put_bits(&pb, 5, 2);    //object type - AAC-LC
    put_bits(&pb, 4, 0x04); // sample rate index, 44100
    put_bits(&pb, 4, 2);    // channel configuration
    //GASpecificConfig
    put_bits(&pb, 1, 0);    // frame length - 1024 samples
    put_bits(&pb, 1, 0);    // does not depend on core coder
    put_bits(&pb, 1, 0);    // is not extension

    flush_put_bits(&pb);

    pbuf += 2;

    return pbuf;
}

// 当收到sps和pps信息时，发送AVC和AAC的sequence header
void RtmpLiveScreen::OnSPSAndPPS(char* spsBuf, int spsSize, char* ppsBuf, int ppsSize)
{
    sps_ = (char*)malloc(spsSize);
    memcpy(sps_, spsBuf, spsSize);
    sps_size_ = spsSize;

    pps_ = (char*)malloc(ppsSize);
    memcpy(pps_, ppsBuf, ppsSize);
    pps_size_ = ppsSize;

    SendAVCSequenceHeaderPacket();
#ifndef ENCODE_AUDIO_USE_MP3
    if (has_audio_)
    {
        SendAACSequenceHeaderPacket();
    }
#endif

    time_begin_ = ::GetTickCount();
    last_timestamp_ = time_begin_;

    gVideoBegin = true;
}

unsigned int RtmpLiveScreen::GetTimestamp()
{
    unsigned int timestamp;
    __int64 now = ::GetTickCount();

    //if (now < last_timestamp_)
    if (now < time_begin_)
    {
        timestamp = 0;
        last_timestamp_ = now;
        time_begin_ = now;
    }
    else
    {
        timestamp = now - time_begin_;
        //timestamp = now - last_timestamp_;
    }
    return timestamp;
}

unsigned int RtmpLiveScreen::GetInputTimestamp()
{
    LASTINPUTINFO li;
    li.cbSize = sizeof(LASTINPUTINFO);
    ::GetLastInputInfo(&li);
    return li.dwTime;
}

bool RtmpLiveScreen::IsNeedReConnect()
{
    return is_send_failed_;
}

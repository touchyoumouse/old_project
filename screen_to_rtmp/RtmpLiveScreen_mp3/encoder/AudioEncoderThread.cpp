#include "stdafx.h"
#include "AudioEncoderThread.h"

#include "dshow/DSCapture.h"
#include "dshow/DSAudioGraph.h"
#include "FAACEncoder.h"
#include "MP3Encoder.h"

AudioEncoderThread::AudioEncoderThread(DSAudioGraph* dsAudioGraph)
    : ds_audio_graph_(dsAudioGraph), //faac_encoder_(new FAACEncoder)
    mp3_encoder_(new MP3Encoder)
{
    // 初始化工作
    mp3_encoder_->Init(ds_audio_graph_->GetSamplesPerSec(), 
        ds_audio_graph_->GetChannels());

    listener_ = NULL;
}

AudioEncoderThread::~AudioEncoderThread()
{
    delete mp3_encoder_;
}

void AudioEncoderThread::Run()
{
    FILE* fp_aac = 0;
    if (false == filename_aac_.empty())
    {
        fp_aac = fopen(filename_aac_.c_str(), "wb");
    }

    // 开始循环获取每一帧，并编码
    unsigned long max_out_bytes = mp3_encoder_->MaxOutBytes(1024);
    unsigned char* outbuf = (unsigned char*)malloc(max_out_bytes);
    unsigned long frame_bytes = mp3_encoder_->FrameSize();

    __int64 last_tick = 0;
    unsigned int timestamp = 0;

    while (false == IsStop())
    {
        char* pcmbuf = ds_audio_graph_->GetBuffer();
        if (ds_audio_graph_->IsBufferAvailable() && pcmbuf && gVideoBegin)
        {
            if (last_tick == 0)
            {
                last_tick = ::GetTickCount();
            }
            else
            {
                __int64 nowtick = ::GetTickCount();
                if (nowtick < last_tick)
                    timestamp = 0;
                else
                    timestamp += (nowtick - last_tick);
                last_tick = nowtick;
            }

            unsigned int sample_count = (ds_audio_graph_->BufferSize() << 3)/ds_audio_graph_->GetBitsPerSample();

            short* audiobuf = (short*)pcmbuf;
            for (unsigned int i = 0; i < sample_count; ++i)
            {
                if (audiobuf[i] > 32767) audiobuf[i] = 32767;
                else if (audiobuf[i] < -32768) audiobuf[i] = -32768;
            }

            unsigned int buf_size = max_out_bytes;
            mp3_encoder_->Encode((unsigned char*)pcmbuf, sample_count,
                (unsigned char*)outbuf, buf_size);

            if (fp_aac && buf_size)
            {
                fwrite(outbuf, buf_size, 1, fp_aac);
            }

            if (listener_ && buf_size)
            {
                base::DataBuffer* dataBuf = new base::DataBuffer((char*)outbuf, buf_size);
                listener_->OnCaptureAudioBuffer(dataBuf, timestamp);
            }
        }
        ds_audio_graph_->ReleaseBuffer(pcmbuf);

        Sleep(1);
    }

    free(outbuf);

    mp3_encoder_->Free();

    if (fp_aac)
    {
        fclose(fp_aac);
    }
}

void AudioEncoderThread::SetOutputFilename(const std::string& filename)
{
    filename_aac_ = filename;
}

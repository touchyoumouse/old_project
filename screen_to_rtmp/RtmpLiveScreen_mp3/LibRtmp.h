#ifndef _LIB_RTMP_H_
#define _LIB_RTMP_H_

#include <string>

#include "librtmp/rtmp.h"
#include "librtmp/log.h"


class LibRtmp
{
public:
    LibRtmp(bool isNeedLog, bool isNeedRecord);

    ~LibRtmp();

    bool Open(const char* url);

    void Close();

    bool Send(const char* buf, int bufLen, int type, unsigned int timestamp);

    void SendSetChunkSize(unsigned int chunkSize);

    void CreateSharedObject();

    void SetSharedObject(bool isSet);

    void SendSharedObject(int val);

private:
	/*RTMPPacket *rtmp_pakt;*/
    RTMP* rtmp_;
    char* streming_url_;
    FILE* flog_;
    bool is_need_record_;
    std::string stream_name_;
public:
	bool control_rtmp_;
};

#endif // _LIB_RTMP_H_

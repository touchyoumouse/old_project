#ifndef _LIB_RTMP_H_
#define _LIB_RTMP_H_

#include "librtmp/rtmp.h"
#include "librtmp/log.h"

class LibRtmp
{
public:
    LibRtmp(bool isNeedLog);

    ~LibRtmp();

    bool Open(const char* url);

    void Close();

    bool Send(const char* buf, int bufLen, int type, unsigned int timestamp);

    bool CopySend(const char* buf, int bufLen, int type, unsigned int timestamp);

    void SendSetChunkSize(unsigned int chunkSize);

    RTMP* GetRTMP() { return rtmp_; }

private:
    RTMP* rtmp_;
    char* streming_url_;
    FILE* flog_;
};

#endif // _LIB_RTMP_H_

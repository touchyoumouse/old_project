#ifndef _DATA_BUFFER_H_
#define _DATA_BUFFER_H_

#include "Base.h"

#include <deque>
#include <set>

#include "Lock.h"

namespace base{

class DataBuffer
{
public:
    long long timestamp_;

public:
    DataBuffer(char* buf, unsigned int bufLen, bool isNeedFree = false)
    {
        buf_ = buf;
        buf_len_ = bufLen;
        is_need_free_ = isNeedFree;
    }

    DataBuffer(const DataBuffer& other)
    {
        buf_ = other.buf_;
        buf_len_ = other.buf_len_;
        is_need_free_ = other.is_need_free_;
    }

    ~DataBuffer()
    {
        if (is_need_free_)
        {
            delete[] buf_;
        }
    }

    char* Buf() { return buf_; }

    unsigned int BufLen() { return buf_len_; }

    DataBuffer* Clone()
    {
        char* buf_clone = new char[buf_len_];
        memcpy(buf_clone, buf_, buf_len_);
        DataBuffer* data_buf = new DataBuffer(buf_clone, buf_len_, true);
        return data_buf;
    }

private:
    char* buf_;
    unsigned int buf_len_;
    bool is_need_free_;
};

class DataBufferCache
{
public:
    DataBufferCache();

    ~DataBufferCache();

    DataBuffer* MallocData(const char* srcData, int srcSize);

    void FreeData(DataBuffer* dataBuffer);

    void Clear();

    int CacheCount();

private:
    std::deque<DataBuffer*> cache_list_;
    std::set<DataBuffer*> using_cache_list_;

    base::Lock list_mtx_;
};

//////////////////////////////////////////////////////////////////////////

class RefCountDataBuffer : public DataBuffer
{
public:
    RefCountDataBuffer(char* buf, unsigned int bufLen, bool isNeedFree = false)
        : DataBuffer(buf, bufLen, isNeedFree)
    {
        ref_count_ = 1;
    }

    RefCountDataBuffer(DataBuffer* dataBuffer)
        : DataBuffer(*dataBuffer)
    {
        ref_count_ = 1;
    }

    virtual ~RefCountDataBuffer()
    {
    }

    int RefCount() const { return ref_count_; }

    void DecRef() { ref_count_--; }

    void ResetRef(int setCount) { ref_count_ = setCount; }

private:
    DISALLOW_COPY_AND_ASSIGN(RefCountDataBuffer);

private:
    int ref_count_;
};

class RefCountDataBufferCache
{
public:
    RefCountDataBufferCache();

    ~RefCountDataBufferCache();

    RefCountDataBuffer* MallocData(const char* srcData, int srcSize);

    void FreeData(RefCountDataBuffer* dataBuffer);

    void Clear();

    int CacheCount();

private:
    std::deque<RefCountDataBuffer*> cache_list_;
    std::set<RefCountDataBuffer*> using_cache_list_;

    base::Lock list_mtx_;
};

} // namespace Base

#endif // _DATA_BUFFER_H_

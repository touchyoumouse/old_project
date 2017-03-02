/*******************************************************************************
 * DataBuffer.cpp
 * Copyright: (c) 2014 Haibin Du(haibindev.cnblogs.com). All rights reserved.
 * -----------------------------------------------------------------------------
 *
 * -----------------------------------------------------------------------------
 * 2015-12-2 9:54 - Created (Haibin Du)
 ******************************************************************************/

#include "DataBuffer.h"

namespace base{

//////////////////////////////////////////////////////////////////////////

DataBufferCache::DataBufferCache()
{

}

DataBufferCache::~DataBufferCache()
{
    Clear();

}

DataBuffer* DataBufferCache::MallocData(const char* srcData,
    int srcSize)
{
    DataBuffer* new_data = NULL;

    if (cache_list_.empty())
    {
        new_data = DataBuffer((char*)srcData, srcSize).Clone();
    }
    else
    {
        new_data = cache_list_.front();

        if (new_data->BufLen() != srcSize)
        {
            return NULL;
        }

        memcpy(new_data->Buf(), srcData, srcSize);
        cache_list_.pop_front();
    }

    using_cache_list_.insert(new_data);

    return new_data;
}

void DataBufferCache::FreeData(DataBuffer* dataBuffer)
{
    using_cache_list_.erase(dataBuffer);

    cache_list_.push_back(dataBuffer);
}

void DataBufferCache::Clear()
{
    for (auto it = cache_list_.begin(); it != cache_list_.end(); ++it)
    {
        DataBuffer* data = *it;

        delete data;
    }
    cache_list_.clear();

    for (auto it = using_cache_list_.begin(); it != using_cache_list_.end(); ++it)
    {
        DataBuffer* data = *it;

        delete data;
    }
    using_cache_list_.clear();
}

int DataBufferCache::CacheCount()
{
    return cache_list_.size() + using_cache_list_.size();
}

//////////////////////////////////////////////////////////////////////////

RefCountDataBufferCache::RefCountDataBufferCache()
{

}

RefCountDataBufferCache::~RefCountDataBufferCache()
{
    Clear();
}

RefCountDataBuffer* RefCountDataBufferCache::MallocData(const char* srcData,
    int srcSize)
{
    RefCountDataBuffer* new_data = NULL;

    if (cache_list_.empty())
    {
        new_data = new RefCountDataBuffer(DataBuffer((char*)srcData, srcSize).Clone());
    }
    else
    {
        new_data = cache_list_.front();

        if (new_data->BufLen() != srcSize)
        {
            return NULL;
        }

        memcpy(new_data->Buf(), srcData, srcSize);

        cache_list_.pop_front();
    }

    using_cache_list_.insert(new_data);

    return new_data;
}

void RefCountDataBufferCache::FreeData(RefCountDataBuffer* dataBuffer)
{
    using_cache_list_.erase(dataBuffer);

    cache_list_.push_back(dataBuffer);
}

void RefCountDataBufferCache::Clear()
{
    for (auto it = cache_list_.begin(); it != cache_list_.end(); ++it)
    {
        RefCountDataBuffer* data = *it;

        delete data;
    }
    cache_list_.clear();

    for (auto it = using_cache_list_.begin(); it != using_cache_list_.end(); ++it)
    {
        RefCountDataBuffer* data = *it;

        delete data;
    }
    using_cache_list_.clear();
}

int RefCountDataBufferCache::CacheCount()
{
    return cache_list_.size() + using_cache_list_.size();
}

} // namespace Base

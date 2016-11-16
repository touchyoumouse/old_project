#include "StdAfx.h"
#include "BufPool.h"
#include <assert.h>

CBufPool::CBufPool(UINT uBlockSize, UINT uInitBufCount)
                   : m_uBlockSize(uBlockSize)
				   , m_uInitBufCount(uInitBufCount)
{
    //判断参数是否合法
    assert(uBlockSize);

   //分配内存
    for (UINT i = 0;i<uInitBufCount;++i)
    {
        char *p = new char[m_uBlockSize];
		if (p)
		{
			m_listFree.push_back(p);
		}        
    }   

    InitializeCriticalSection(&m_criBufPool);
}

CBufPool::~CBufPool(void)
{
    EnterCriticalSection(&m_criBufPool);
    ITLISTMEMERY it;
	//释放空闲队列内存
	if (!m_listFree.empty())
	{
		for(it = m_listFree.begin();it != m_listFree.end();++it)
		{

			if (*it)
			{
				delete[] *it;
			}
		} 
		m_listFree.clear();
	}
    
	//释放使用状态的内存
	if (!m_listUsed.empty())
	{
		for(it = m_listUsed.begin();it != m_listUsed.end();++it)
		{

			if (*it)
			{
				delete[] *it;
			}
		} 
		m_listUsed.clear();
	}

    LeaveCriticalSection(&m_criBufPool);
	//删除临界区
    DeleteCriticalSection(&m_criBufPool);
}

//////////////////////////////////////////////////////////////////////////
//请求分配一个buf
//参数：无
//返回值：分配成功，返回指向分配的buf指针，否则返回NULL
char * CBufPool::NewBuf()
{	
    char * pRet = NULL;
    EnterCriticalSection(&m_criBufPool);
	//判断当前是否有空闲内存块
	if(!m_listFree.empty())
	{
		//如果有，则从空闲队列头取出一块内存
		ITLISTMEMERY it = m_listFree.begin();
		pRet = *it;
		m_listFree.erase(it);
		//把内存块放到使用队列中
		m_listUsed.push_back(pRet);
	}
	else
	{
		//如果没有空闲的内存，则分配一块
		pRet = new char[m_uBlockSize];
		if (!pRet)
		{
			Sleep(1);
			pRet = new char[m_uBlockSize];
		}

		//判断是否分配成功
		if (pRet)
		{
			//分配成功则将内存块放到使用队列中
			m_listUsed.push_back(pRet);
		}
	}	

    LeaveCriticalSection(&m_criBufPool);
    return pRet;
}
//////////////////////////////////////////////////////////////////////////
//释放一个buf
//参数：buf的指针
//返回值：无
void CBufPool::FreeBuf(char * pBuf)
{
    if (!pBuf)
    {
        return;
    }

    EnterCriticalSection(&m_criBufPool);
	//在使用队列中查找要释放的内存块
    ITLISTMEMERY it = m_listUsed.begin();	
    for(;it != m_listUsed.end();++it)
    {
        if ((*it) == pBuf)
        {
			//将内存块从使用队列中删除
			m_listUsed.erase(it);
			//判断当前空闲队列是否大于初始值
			if (m_listFree.size() < m_uInitBufCount)
			{
				//如果小于初始值，则将内存块放到空闲队列
				m_listFree.push_back(pBuf);
			}
			else
			{
				//判断当前空闲队列大于或等于初始值，则将内存块释放
				delete[] pBuf;
			}
			
            break;
        }
    } 
    LeaveCriticalSection(&m_criBufPool);
}
//////////////////////////////////////////////////////////////////////////
//释放所有buf
//参数：无
//返回值：无
void CBufPool::FreeAllBuf()
{
    EnterCriticalSection(&m_criBufPool);	
    ITLISTMEMERY it = m_listUsed.begin();
    for(;it != m_listUsed.end() && m_listFree.size() < m_uInitBufCount ;++it)
    {        
        m_listFree.push_back(*it);
    } 

	for(;it != m_listUsed.end();++it)
	{        
		delete [] (*it);       
	}

	m_listUsed.clear();
    LeaveCriticalSection(&m_criBufPool);
}

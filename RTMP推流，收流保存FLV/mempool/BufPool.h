//////////////////////////////////////////////////////////////////////////
//简单内存池 
//具有线程安全
// V 1.0    吴智杰     2008.4.12
//可以一次分配同样大小的多块内存
// v 1.5	吴智杰     2008.4.18	修改为缓存队列，并及时释放多余的空间

#pragma once
#include <list>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
using namespace std;

//typedef struct __MemeryInfo{
//    BYTE bUsingStat;
//    char * pBlock;
//} MEMINFO, *PMEMINFO;
//
typedef list<char *> LISTMEMERY;
typedef list<char *>::iterator ITLISTMEMERY;

class CBufPool
{
public:
 
    //////////////////////////////////////////////////////////////////////////
    //构造函数
    //参数：   uBlockSize   每块内存大小
    //          uInitBufCount 初始申请内存块数
    //返回值：分配成功，返回指向分配的buf指针，否则返回NULL
    CBufPool(UINT uBlockSize, UINT uInitBufCount = 5);

    //////////////////////////////////////////////////////////////////////////
    //析构函数
    //参数：无
    //返回值：无
    ~CBufPool(void);

    //////////////////////////////////////////////////////////////////////////
    //请求分配一个buf
    //参数：无
    //返回值：分配成功，返回指向分配的buf指针，否则返回NULL
    char * NewBuf();

    //////////////////////////////////////////////////////////////////////////
    //释放一个buf
    //参数：buf的指针
    //返回值：无
    void FreeBuf(char * pBuf);

    //////////////////////////////////////////////////////////////////////////
    //释放所有buf
    //参数：无
    //返回值：无
    void FreeAllBuf();

    //////////////////////////////////////////////////////////////////////////
    //获取每个buf的大小
    //参数：无
    //返回值：每个buf的大小
    UINT GetSizePerBuf(){return m_uBlockSize;};
    

private:
    UINT                m_uBlockSize;		//每个缓存的大小
	UINT                m_uInitBufCount;	//每个缓存的大小
    CRITICAL_SECTION    m_criBufPool;       //缓存临界区
    LISTMEMERY          m_listFree;			//空闲的内存块
	LISTMEMERY			m_listUsed;			//使用中的内存块

    //buf占用状态值
    enum{
        BufFree = 0,//空闲
        BufUsed = 1 //在使用中
    };
};

#include "StdAfx.h"
#include "BufPool.h"
#include <assert.h>

CBufPool::CBufPool(UINT uBlockSize, UINT uInitBufCount)
                   : m_uBlockSize(uBlockSize)
				   , m_uInitBufCount(uInitBufCount)
{
    //�жϲ����Ƿ�Ϸ�
    assert(uBlockSize);

   //�����ڴ�
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
	//�ͷſ��ж����ڴ�
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
    
	//�ͷ�ʹ��״̬���ڴ�
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
	//ɾ���ٽ���
    DeleteCriticalSection(&m_criBufPool);
}

//////////////////////////////////////////////////////////////////////////
//�������һ��buf
//��������
//����ֵ������ɹ�������ָ������bufָ�룬���򷵻�NULL
char * CBufPool::NewBuf()
{	
    char * pRet = NULL;
    EnterCriticalSection(&m_criBufPool);
	//�жϵ�ǰ�Ƿ��п����ڴ��
	if(!m_listFree.empty())
	{
		//����У���ӿ��ж���ͷȡ��һ���ڴ�
		ITLISTMEMERY it = m_listFree.begin();
		pRet = *it;
		m_listFree.erase(it);
		//���ڴ��ŵ�ʹ�ö�����
		m_listUsed.push_back(pRet);
	}
	else
	{
		//���û�п��е��ڴ棬�����һ��
		pRet = new char[m_uBlockSize];
		if (!pRet)
		{
			Sleep(1);
			pRet = new char[m_uBlockSize];
		}

		//�ж��Ƿ����ɹ�
		if (pRet)
		{
			//����ɹ����ڴ��ŵ�ʹ�ö�����
			m_listUsed.push_back(pRet);
		}
	}	

    LeaveCriticalSection(&m_criBufPool);
    return pRet;
}
//////////////////////////////////////////////////////////////////////////
//�ͷ�һ��buf
//������buf��ָ��
//����ֵ����
void CBufPool::FreeBuf(char * pBuf)
{
    if (!pBuf)
    {
        return;
    }

    EnterCriticalSection(&m_criBufPool);
	//��ʹ�ö����в���Ҫ�ͷŵ��ڴ��
    ITLISTMEMERY it = m_listUsed.begin();	
    for(;it != m_listUsed.end();++it)
    {
        if ((*it) == pBuf)
        {
			//���ڴ���ʹ�ö�����ɾ��
			m_listUsed.erase(it);
			//�жϵ�ǰ���ж����Ƿ���ڳ�ʼֵ
			if (m_listFree.size() < m_uInitBufCount)
			{
				//���С�ڳ�ʼֵ�����ڴ��ŵ����ж���
				m_listFree.push_back(pBuf);
			}
			else
			{
				//�жϵ�ǰ���ж��д��ڻ���ڳ�ʼֵ�����ڴ���ͷ�
				delete[] pBuf;
			}
			
            break;
        }
    } 
    LeaveCriticalSection(&m_criBufPool);
}
//////////////////////////////////////////////////////////////////////////
//�ͷ�����buf
//��������
//����ֵ����
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

//////////////////////////////////////////////////////////////////////////
//���ڴ�� 
//�����̰߳�ȫ
// V 1.0    ���ǽ�     2008.4.12
//����һ�η���ͬ����С�Ķ���ڴ�
// v 1.5	���ǽ�     2008.4.18	�޸�Ϊ������У�����ʱ�ͷŶ���Ŀռ�

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
    //���캯��
    //������   uBlockSize   ÿ���ڴ��С
    //          uInitBufCount ��ʼ�����ڴ����
    //����ֵ������ɹ�������ָ������bufָ�룬���򷵻�NULL
    CBufPool(UINT uBlockSize, UINT uInitBufCount = 5);

    //////////////////////////////////////////////////////////////////////////
    //��������
    //��������
    //����ֵ����
    ~CBufPool(void);

    //////////////////////////////////////////////////////////////////////////
    //�������һ��buf
    //��������
    //����ֵ������ɹ�������ָ������bufָ�룬���򷵻�NULL
    char * NewBuf();

    //////////////////////////////////////////////////////////////////////////
    //�ͷ�һ��buf
    //������buf��ָ��
    //����ֵ����
    void FreeBuf(char * pBuf);

    //////////////////////////////////////////////////////////////////////////
    //�ͷ�����buf
    //��������
    //����ֵ����
    void FreeAllBuf();

    //////////////////////////////////////////////////////////////////////////
    //��ȡÿ��buf�Ĵ�С
    //��������
    //����ֵ��ÿ��buf�Ĵ�С
    UINT GetSizePerBuf(){return m_uBlockSize;};
    

private:
    UINT                m_uBlockSize;		//ÿ������Ĵ�С
	UINT                m_uInitBufCount;	//ÿ������Ĵ�С
    CRITICAL_SECTION    m_criBufPool;       //�����ٽ���
    LISTMEMERY          m_listFree;			//���е��ڴ��
	LISTMEMERY			m_listUsed;			//ʹ���е��ڴ��

    //bufռ��״ֵ̬
    enum{
        BufFree = 0,//����
        BufUsed = 1 //��ʹ����
    };
};

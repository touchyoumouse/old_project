#include "Lock.h"

namespace base {

Lock::Lock()
{
#ifdef WIN32
    // 2000 ָ�����ڵȴ������ٽ���ʱ������������ֹ�߳�˯��
    ::InitializeCriticalSectionAndSpinCount(&mtx_, 4000);
#else
    pthread_mutex_init(&mtx_, NULL);
#endif
}

Lock::~Lock()
{
#ifdef WIN32
    ::DeleteCriticalSection(&mtx_);
#else
    pthread_mutex_destroy(&mtx_);
#endif
}

bool Lock::Try()
{
#ifdef WIN32
    if (::TryEnterCriticalSection(&mtx_) != FALSE)
    {
        return true;
    }
#else
#endif
    return false;
}

void Lock::Acquire()
{
#ifdef WIN32
    ::EnterCriticalSection(&mtx_);
#else
    pthread_mutex_lock(&mtx_);
#endif
}

void Lock::Release()
{
#ifdef WIN32
    ::LeaveCriticalSection(&mtx_);
#else
    pthread_mutex_unlock(&mtx_);
#endif
}

}

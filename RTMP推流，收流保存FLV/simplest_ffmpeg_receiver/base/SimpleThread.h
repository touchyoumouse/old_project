#ifndef _SIMPLE_THREAD_H_
#define _SIMPLE_THREAD_H_

#include "Base.h"

#ifdef WIN32
#define THREAD_FUNC_TYPE void
#define THREAD_FUNC_RETURN()
typedef HANDLE thread_handle_t;
#else
#define THREAD_FUNC_TYPE void*
#define THREAD_FUNC_RETURN() return 0
typedef pthread_t thread_handle_t;
#endif

namespace base
{

class SimpleThread
{
public:
    SimpleThread();

    virtual ~SimpleThread();

    // �����߳�, ��֤Start()��������ʱ���߳��Ѿ���ʼִ��
    virtual void ThreadStart();

    virtual void ThreadStop();

    virtual bool IsThreadStop();

    // �ȴ��̹߳ر�
    virtual void ThreadJoin();

    // ������Ҫ���ش˺���
    // Run()�����������½����߳���
    virtual void ThreadRun() = 0;

    thread_handle_t ThreadId() { return thread_id_; }

    void ThreadMain();

private:
    thread_handle_t thread_id_;
    volatile bool is_stop_;
};

} // namespace Thread

#endif // _SIMPLE_THREAD_H_

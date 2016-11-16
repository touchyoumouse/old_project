#ifndef _LOCK_H_
#define _LOCK_H_

//#include "Base.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>


#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
	TypeName(const TypeName&);               \
	void operator=(const TypeName&)




namespace base{

	class Lock
	{
	public:
		Lock();

		~Lock();

		bool Try();

		void Acquire();

		void Release();

	private:
#ifdef WIN32
		CRITICAL_SECTION mtx_;
#else
		pthread_mutex_t mtx_;
#endif

		DISALLOW_COPY_AND_ASSIGN(Lock);
	};

	//
	// Lock������
	// ���캯��-����
	// ��������-�ͷ���
	//
	class AutoLock {
	public:
		explicit AutoLock(Lock& lock) : lock_(lock) {
			lock_.Acquire();
		}

		~AutoLock() {
			lock_.Release();
		}

	private:
		Lock& lock_;
		DISALLOW_COPY_AND_ASSIGN(AutoLock);
	};

	//
	// Lock������
	// ���캯��-�ͷ���
	// ��������-����
	//
	class AutoUnlock {
	public:
		explicit AutoUnlock(Lock& lock) : lock_(lock) {
			lock_.Release();
		}

		~AutoUnlock() {
			lock_.Acquire();
		}

	private:
		Lock& lock_;
		DISALLOW_COPY_AND_ASSIGN(AutoUnlock);
	};

} // namespace Base

#endif // _LOCK_H_

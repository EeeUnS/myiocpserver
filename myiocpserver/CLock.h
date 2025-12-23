#pragma once 

#include <windows.h>

class CLock
{
public:
	CLock();
	~CLock();
	void Lock();
	void Unlock();
private:
	SRWLOCK m_lock;
	int m_nRecursiveCount;
	DWORD m_dwThreadId; // Lock을 소유한 스레드 ID
};

struct SAutoLockHelper
{
	CLock& m_cl;
	SAutoLockHelper(CLock& cl);
	~SAutoLockHelper();
};


#define AUTO_LOCK(cl) if ( SAutoLockHelper _autoLockHelper_##cl(cl); true )
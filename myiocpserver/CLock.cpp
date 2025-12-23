#include "pch.h"
#include "CLock.h"
#include "DebugHelper.h"


#pragma warning(disable: 26110) // warning C26110: 'CLock::m_lock'이(가) 올바르게 초기화되지 않았습니다. 멤버 변수가 초기화되지 않은 상태로 사용될 수 있습니다.

CLock::CLock()
{
    InitializeSRWLock(&m_lock);
	m_dwThreadId = 0;
	m_nRecursiveCount = 0;
}

CLock::~CLock()
{
    // SRWLOCK does not require explicit destruction
}

void CLock::Lock()
{
	if (m_nRecursiveCount == 0)
	{
		ASSERT(0 == m_dwThreadId);
		AcquireSRWLockExclusive(&m_lock);
		m_dwThreadId = GetCurrentThreadId();
	}
	else
	{
		DWORD curId = GetCurrentThreadId();
		ASSERT(m_dwThreadId == curId);

		++m_nRecursiveCount;
	}
}

void CLock::Unlock()
{
	DWORD curId = GetCurrentThreadId();
	ASSERT(0 != m_dwThreadId && m_dwThreadId == curId);

	if (m_nRecursiveCount == 0)
	{
		ReleaseSRWLockExclusive(&m_lock);
		m_dwThreadId = 0;
	}
	else
	{
		--m_nRecursiveCount;
	}
}

#pragma warning(default: 26110) 

SAutoLockHelper::SAutoLockHelper(CLock& cl) : m_cl(cl)
{
	m_cl.Lock();
}

SAutoLockHelper::~SAutoLockHelper()
{
	m_cl.Unlock();
}

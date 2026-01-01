#include "pch.h"
#include "Thread.h"
#include <process.h>
#include "DebugHelper.h"

CThread::CThread()
	: m_hThread(NULL), m_dwThreadId(0), m_bRunning(false)
{
}

CThread::~CThread()
{
	DestroyThread();
}

bool CThread::CreateThread()
{
	ASSERT(m_hThread == NULL);

	unsigned threadId = 0;
	m_hThread = (HANDLE)_beginthreadex(
		NULL,
		0,
		ThreadFunc,
		this,
		0,
		&threadId);
	ASSERT(m_hThread != NULL);
	if (m_hThread == NULL) {
		return false;
	}
	m_dwThreadId = threadId;
	m_bRunning = true;
	return true;
}

void CThread::DestroyThread()
{
	ASSERT(m_hThread != NULL);

	m_bRunning = false;

	WaitForSingleObject(m_hThread, INFINITE);
	CloseHandle(m_hThread);
	m_hThread = NULL;
}

DWORD CThread::ThreadProc()
{
	// override in derived class
	return 0;
}

unsigned WINAPI CThread::ThreadFunc(void* lpParam)
{
	CThread* pThis = static_cast<CThread*>(lpParam);
	return pThis->ThreadProc();
}

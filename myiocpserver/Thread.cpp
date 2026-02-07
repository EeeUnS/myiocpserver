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

void CThread::DestroyThread()
{
	ASSERT(m_hThread != NULL);

	m_bRunning = false;

	WaitForSingleObject(m_hThread, INFINITE);
	CloseHandle(m_hThread);
	m_hThread = NULL;
}

unsigned WINAPI CThread::ThreadFunc(void* lpParam)
{
	CThread* pThis = static_cast<CThread*>(lpParam);
	return pThis->FuncImplRun();
}

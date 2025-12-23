#pragma once

#include <windows.h>

class CThread
{
public:
	CThread();
	virtual ~CThread();
	bool CreateThread();
	void DestroyThread();
protected:
	virtual DWORD ThreadProc();
private:
	static unsigned WINAPI ThreadFunc(LPVOID lpParam);
	HANDLE m_hThread;
	DWORD m_dwThreadId;
	bool m_bRunning;
};
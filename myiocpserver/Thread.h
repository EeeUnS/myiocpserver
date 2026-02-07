#pragma once

#include <windows.h>
#include <tuple>
#include <utility>

class CThread
{
protected:
	// 인자를 전달하기위해 스레드 생성 시 호출되는 함수, 파생 클래스에서 필요시 구현
	virtual void OnThreadCreate() {}

	// 스레드 작동 함수, 파생 클래스에서 구현
	virtual DWORD FuncImplRun() = 0;
public:
	CThread();
	virtual ~CThread();
	
	template<typename... Args>
	bool CreateThread(Args&&... args)
	{
		ASSERT(m_hThread == NULL);
		
		OnThreadCreate(std::forward<Args>(args)...);
		
		unsigned threadId = 0;
		m_hThread = (HANDLE)_beginthreadex(
			NULL,
			0,
			ThreadFunc,
			this,
			0,
			&threadId);
		ASSERT(m_hThread != NULL);
		if (m_hThread == NULL)
		{
			return false;
		}
		m_dwThreadId = threadId;
		m_bRunning = true;
		return true;
	}
	
	void DestroyThread();
	
	
private:
	static unsigned WINAPI ThreadFunc(LPVOID lpParam);
	HANDLE m_hThread;
	DWORD m_dwThreadId;
	bool m_bRunning;
};
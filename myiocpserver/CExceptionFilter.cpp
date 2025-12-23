#include "CExceptionFilter.h"

#include <Windows.h>
#include <thread>
#include <Dbghelp.h>
#include <libloaderapi.h>
#include <exception>

#include <cstdlib>
#include <csignal>
#include <csignal>

//https://docs.proudnet.com/ProudNet/pn_reference_ko/notes/minidump
#pragma comment(lib, "Dbghelp.lib")


MINIDUMP_TYPE CExceptionFilter::m_eDumpType = static_cast<MINIDUMP_TYPE>(MiniDumpWithDataSegs | MiniDumpWithHandleData | MiniDumpWithThreadInfo);

static void Access_Violation()
{
	RaiseException(EXCEPTION_ACCESS_VIOLATION, EXCEPTION_NONCONTINUABLE, 0, NULL);
}

void CExceptionFilter::SetExceptionFilter()
{
	_set_purecall_handler(Access_Violation);
	_set_invalid_parameter_handler([](wchar_t const*, wchar_t const*, wchar_t const*, unsigned int, uintptr_t)
		{
			Access_Violation();
		});

	signal(SIGABRT, [](int)
		{
			Access_Violation();
		});

	set_terminate(Access_Violation);

	::std::set_terminate([]()
		{
			Access_Violation();
		});

	SetUnhandledExceptionFilter(UnHandledExceptionFilter);
}

void CExceptionFilter::SetDumpType(MINIDUMP_TYPE edumpType)
{
	m_eDumpType = edumpType;
}

//SetUnhandledExceptionFilter(UnHandledExceptionFilter);
LONG WINAPI CExceptionFilter::UnHandledExceptionFilter(PEXCEPTION_POINTERS pExceptionInfo)
{
	if (pExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_STACK_OVERFLOW)
	{
		// 스택 오버플로우는 별도 스레드에서 처리
		std::thread overflowThread = std::thread(WriteDump, pExceptionInfo);
		overflowThread.join();
	}
	else
	{
		return WriteDump(pExceptionInfo);  // 일반 예외는 바로 처리
	}
	return 0;
}

DWORD WINAPI CExceptionFilter::WriteDump(LPVOID pExceptionInfoOrNull)
{
	TCHAR sDumpName[MAX_PATH] = { NULL, };
	{
		SYSTEMTIME SystemTime;
		ZeroMemory(&SystemTime, sizeof(SystemTime));
		GetLocalTime(&SystemTime);

		TCHAR processName[MAX_PATH] = { NULL, };
		Util::TryGetProcessName(processName);

		_snwprintf_s(sDumpName, MAX_PATH, TEXT("%s_%02d%02d%02d_%02d%02d%02d.dmp"),
			processName,
			SystemTime.wYear % 1000, SystemTime.wMonth, SystemTime.wDay,
			SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond
		);
	}

	HANDLE hFile = CreateFile(sDumpName, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		return 0;
	}

	MINIDUMP_EXCEPTION_INFORMATION sttInfo;
	sttInfo.ThreadId = ::GetCurrentThreadId();
	sttInfo.ExceptionPointers = (PEXCEPTION_POINTERS)pExceptionInfoOrNull;
	sttInfo.ClientPointers = FALSE;

	MINIDUMP_EXCEPTION_INFORMATION* pExceptionInfo = nullptr;
	if (pExceptionInfoOrNull != nullptr)
	{
		pExceptionInfo = &sttInfo;
	}


	MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, m_eDumpType, pExceptionInfo, NULL, NULL);

	CloseHandle(hFile);
	return 0;
}


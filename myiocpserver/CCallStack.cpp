#include "CCallStack.h"
#include <windows.h>
#include <dbghelp.h>
#include <psapi.h>

#pragma comment(lib, "dbghelp.lib")
#pragma comment(lib, "psapi.lib")

__declspec(thread) WCHAR TLS_BUFFER[4096] = { 0, };

void CCallStack::LogCallStack()
{
	// 버퍼 초기화
	TLS_BUFFER[0] = 0;

	HANDLE hProcess = GetCurrentProcess();
	HANDLE hThread = GetCurrentThread();

	TCHAR slogFileName[MAX_PATH] = { 0 };
	DWORD len = GetModuleFileNameW(NULL, slogFileName, MAX_PATH);
	if (len == 0 || len == MAX_PATH)
	{
		lstrcpy(slogFileName, L"UnknownProcess.log");
	}
	else
	{
		// slogFileName에는 전체 경로가 들어있음
		TCHAR* p = slogFileName + len;
		while (p > slogFileName && *p != L'.' && *p != L'\\' && *p != L'/')
		{
			--p;
		}
		if (*p == L'.')
		{
			*p = 0; // 확장자 제거
		}

		lstrcat(slogFileName, L".log");
	}

	// 파일 열기
	HANDLE hFile = CreateFileW(
		slogFileName,
		FILE_APPEND_DATA,
		FILE_SHARE_READ,
		NULL,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		return;
	}

	// 파일이 새로 생성된 경우에만 BOM 작성
	LARGE_INTEGER fileSize = {};
	if (GetFileSizeEx(hFile, &fileSize) && fileSize.QuadPart == 0)
	{
		unsigned short bom = 0xFEFF;
		WriteFile(hFile, &bom, sizeof(bom), NULL, NULL);
	}

	DWORD written;

	// 날짜/시간 기록		
	SYSTEMTIME st;
	GetLocalTime(&st);
	wsprintf(TLS_BUFFER, L"================DateTime: %04d-%02d-%02d %02d:%02d:%02d.%03d================ \r\n",
		st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	WriteFile(hFile, TLS_BUFFER, wcslen(TLS_BUFFER) * sizeof(WCHAR), &written, NULL);

	SymInitialize(hProcess, NULL, TRUE);

	CONTEXT context = {};
	RtlCaptureContext(&context);

	// 레지스터 값 기록 (x64/x86)
#if defined(_M_X64)
	wsprintf(TLS_BUFFER, L"Registers:\r\n"
		L"RAX=%016I64X RBX=%016I64X RCX=%016I64X RDX=%016I64X\r\n"
		L"RSI=%016I64X RDI=%016I64X RBP=%016I64X RSP=%016I64X\r\n"
		L"R8 =%016I64X R9 =%016I64X R10=%016I64X R11=%016I64X\r\n"
		L"R12=%016I64X R13=%016I64X R14=%016I64X R15=%016I64X\r\n"
		L"RIP=%016I64X\r\n\r\n\r\n",
		context.Rax, context.Rbx, context.Rcx, context.Rdx,
		context.Rsi, context.Rdi, context.Rbp, context.Rsp,
		context.R8, context.R9, context.R10, context.R11,
		context.R12, context.R13, context.R14, context.R15,
		context.Rip);
	WriteFile(hFile, TLS_BUFFER, wcslen(TLS_BUFFER) * sizeof(WCHAR), &written, NULL);
#elif defined(_M_IX86)
	wsprintf(TLS_BUFFER, L"Registers:\r\n"
		L"EAX=%08X EBX=%08X ECX=%08X EDX=%08X\r\n"
		L"ESI=%08X EDI=%08X EBP=%08X ESP=%08X\r\n"
		L"EIP=%08X\r\n\r\n\r\n",
		context.Eax, context.Ebx, context.Ecx, context.Edx,
		context.Esi, context.Edi, context.Ebp, context.Esp,
		context.Eip);
	WriteFile(hFile, TLS_BUFFER, wcslen(TLS_BUFFER) * sizeof(WCHAR), NULL, NULL);
#endif

	STACKFRAME64 stackFrame = {};
#if defined(_M_X64)
	DWORD machineType = IMAGE_FILE_MACHINE_AMD64;
	stackFrame.AddrPC.Offset = context.Rip;
	stackFrame.AddrPC.Mode = AddrModeFlat;
	stackFrame.AddrFrame.Offset = context.Rbp;
	stackFrame.AddrFrame.Mode = AddrModeFlat;
	stackFrame.AddrStack.Offset = context.Rsp;
	stackFrame.AddrStack.Mode = AddrModeFlat;
#elif defined(_M_IX86)
	DWORD machineType = IMAGE_FILE_MACHINE_I386;
	stackFrame.AddrPC.Offset = context.Eip;
	stackFrame.AddrPC.Mode = AddrModeFlat;
	stackFrame.AddrFrame.Offset = context.Ebp;
	stackFrame.AddrFrame.Mode = AddrModeFlat;
	stackFrame.AddrStack.Offset = context.Esp;
	stackFrame.AddrStack.Mode = AddrModeFlat;
#else
	static_assert(false, "Unsupported platform");
#endif

	wsprintf(TLS_BUFFER, L"Callstack:\r\n");
	WriteFile(hFile, TLS_BUFFER, lstrlen(TLS_BUFFER) * sizeof(WCHAR), &written, NULL);


	for (int frame = 0; frame < 64; ++frame)
	{
		BOOL result = StackWalk64(
			machineType,
			hProcess,
			hThread,
			&stackFrame,
			&context,
			NULL,
			SymFunctionTableAccess64,
			SymGetModuleBase64,
			NULL);

		if (!result || stackFrame.AddrPC.Offset == 0)
		{
			break;
		}

		DWORD64 addr = stackFrame.AddrPC.Offset;

		char sSymbolBuffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME] = {};
		PSYMBOL_INFO pSymbol = reinterpret_cast<PSYMBOL_INFO>(sSymbolBuffer);
		pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
		pSymbol->MaxNameLen = MAX_SYM_NAME;

		DWORD64 displacement = 0;
		IMAGEHLP_LINE64 lineInfo = {};
		lineInfo.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
		DWORD lineDisplacement = 0;

		// 모듈명 구하기
		DWORD64 moduleBase = SymGetModuleBase64(hProcess, addr);
		WCHAR moduleName[MAX_PATH] = L"";
		WCHAR* pFile = moduleName;
		if (moduleBase)
		{
			GetModuleFileNameW((HMODULE)moduleBase, moduleName, MAX_PATH);
			for (WCHAR* p = moduleName; *p; ++p)
			{
				if (*p == L'\\' || *p == L'/')
				{
					pFile = p + 1;
				}
			}
		}
		else
		{
			pFile = (WCHAR*)L"(unknown)";
		}

		if (SymFromAddr(hProcess, addr, &displacement, pSymbol))
		{
			if (SymGetLineFromAddr64(hProcess, addr, &lineDisplacement, &lineInfo) && lineInfo.FileName)
			{
				wsprintf(TLS_BUFFER, L"    %016I64X: %S (%S:%d) [%s]\r\n", addr, pSymbol->Name, lineInfo.FileName, lineInfo.LineNumber, pFile);
			}
			else
			{
				wsprintf(TLS_BUFFER, L"    %016I64X: %S [%s]\r\n", addr, pSymbol->Name, pFile);
			}
		}
		else
		{
			wsprintf(TLS_BUFFER, L"    %016I64X: (no symbol) [%s]\r\n", addr, pFile);
		}
		WriteFile(hFile, TLS_BUFFER, wcslen(TLS_BUFFER) * sizeof(WCHAR), &written, NULL);
	}

	wsprintf(TLS_BUFFER, L"\r\n");
	WriteFile(hFile, TLS_BUFFER, wcslen(TLS_BUFFER) * sizeof(WCHAR), &written, NULL);

	CloseHandle(hFile);
	SymCleanup(hProcess);
}

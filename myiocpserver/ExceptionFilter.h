#pragma once

#include <Windows.h>
#include<thread>
#include <Dbghelp.h>
#include <libloaderapi.h>

class CExceptionFilter
{
    static MINIDUMP_TYPE m_eDumpType;
    static LONG WINAPI UnHandledExceptionFilter(PEXCEPTION_POINTERS pExceptionInfo);
public:
    static void SetDumpType(MINIDUMP_TYPE edumpType);
    static void SetExceptionFilter();
    static DWORD WINAPI WriteDump(LPVOID pExceptionInfo);
};

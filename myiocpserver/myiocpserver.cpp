#include "pch.h"
#include "IOCompletionPort.hpp"
#include "ExceptionFilter.h"


enum { SERVER_PORT = 11021 };
enum { MAX_CLIENT = 100 };

typedef ULONGLONG ptr;


size_t GetProcessCacheLine()
{
	SYSTEM_LOGICAL_PROCESSOR_INFORMATION buffer[256];
	DWORD returnLength = sizeof(buffer);
	if (!GetLogicalProcessorInformation(buffer, &returnLength))
	{
		return 0;
	}
	size_t cacheLineSize = 0;
	DWORD count = returnLength / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
	for (DWORD i = 0; i < count; ++i)
	{
		if (buffer[i].Relationship == RelationCache)
		{
			PCACHE_DESCRIPTOR cache = &buffer[i].Cache;
			if (cache->Level == 1) // L1 Cache
			{
				cacheLineSize = cache->LineSize;
				break;
			}
		}
	}
	return cacheLineSize;
}

int main()
{
	CExceptionFilter::SetExceptionFilter();

	size_t compileTimeCacheLine = std::hardware_destructive_interference_size;
	
	ASSERT(compileTimeCacheLine >= GetProcessCacheLine());


	CIOCompletionPort ioCompletionPort;

	ioCompletionPort.InitSocket();
	//DEBUGBREAK;
	ioCompletionPort.BindandListen(SERVER_PORT);

	ioCompletionPort.StartServer(MAX_CLIENT, 4);

	ioCompletionPort.DestroyThread();
	return 0;
}

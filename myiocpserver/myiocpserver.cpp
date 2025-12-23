#include "pch.h"
#include "IOCompletionPort.hpp"
#include "CExceptionFilter.h"


enum { SERVER_PORT = 11021 };
enum { MAX_CLIENT = 100 };

typedef ULONGLONG ptr;


int main()
{
	CExceptionFilter::SetExceptionFilter();

	IOCompletionPort ioCompletionPort;

	ioCompletionPort.InitSocket();
	//DEBUGBREAK;
	ioCompletionPort.BindandListen(SERVER_PORT);

	ioCompletionPort.StartServer(MAX_CLIENT);

	ioCompletionPort.DestroyThread();
	return 0;
}

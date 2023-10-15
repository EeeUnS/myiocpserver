#include "IOCompletionPort.hpp"
#include <map>
#include <iostream>
#include <array>

constexpr UINT16 SERVER_PORT = 11021;
constexpr UINT16 MAX_CLIENT = 100;

int main()
{
	IOCompletionPort ioCompletionPort;

	ioCompletionPort.InitSocket();
	DEBUGBREAK;
	ioCompletionPort.BindandListen(SERVER_PORT);

	ioCompletionPort.StartServer(MAX_CLIENT);

	getchar();

	ioCompletionPort.DestroyThread();

	return 0;
}

#pragma once
#pragma comment (lib, "ws2_32")

#define ASSERT( _val) { if (!_val) { DebugBreak();}}
#define DEBUGBREAK ASSERT(false)

#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <iostream>
#include <wchar.h>
#include <vector>
#include <thread>

constexpr int MAX_SOCKBUF = 1024;
constexpr int MAX_WORKERTHREAD = 4;

enum class IOOperation
{
	RECV,
	SEND
};

struct stOverlappedEx
{
	WSAOVERLAPPED m_wsaOverlapped;
	SOCKET m_socketClient;
	WSABUF m_wsaBuf;
	char m_szBuf[MAX_SOCKBUF];
	IOOperation m_eOperation;
};

struct stClientInfo
{
	SOCKET m_socketClient;
	stOverlappedEx m_stRecvOverlappedEx;
	stOverlappedEx m_stSendOverlappedEx;

	stClientInfo()
	{
		ZeroMemory(&m_stRecvOverlappedEx, sizeof(stOverlappedEx));
		ZeroMemory(&m_stSendOverlappedEx, sizeof(stOverlappedEx));
		m_socketClient = INVALID_SOCKET;
	}
};


class IOCompletionPort
{
public:
	IOCompletionPort() = default;
	~IOCompletionPort();

	bool InitSocket();

	bool BindandListen(int nBindPort);

	bool StartServer(const UINT32 maxClientCount);

	void DestroyThread();


private:
	HANDLE mIOCPHandle;
	SOCKET mListenSocket;
	std::vector<stClientInfo> mClientInfos;
	std::vector<std::thread> mIOWorkerThreads;
	std::thread mAccepterThread;
	bool mIsWorkerRun = true;
	bool mIsAccepterRun = true;

	stClientInfo* getEmptyClientInfo();

	bool bindIOCompletionPort(stClientInfo* pClientInfo);

	void ioworkerthread();

	bool sendMsg(stClientInfo* pClientInfo, char* pMsg, int nLen);

	void closeSocket(stClientInfo* pClientInfo, bool bIsForce = false);

	void acceptorThread();

	bool bindRecv(stClientInfo* pClientInfo);

	void createClient(const UINT32 maxClientCount);

	bool createIOWorkerThread();

	bool createAcceptorThread();
};
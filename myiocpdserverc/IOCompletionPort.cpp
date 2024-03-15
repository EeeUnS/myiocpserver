#include "IOCompletionPort.hpp"

IOCompletionPort::~IOCompletionPort()
{
	WSACleanup();
}

bool IOCompletionPort::InitSocket()
{
	WSADATA wsaData;

	int ret = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (ret != 0)
	{
		MessageBox(nullptr, L"윈속 초기화 실패", L"알림", MB_OK);
		ASSERT(false);
		return false;
	}

	mListenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);

	if (mListenSocket == INVALID_SOCKET)
	{
		wchar_t errorStr[64];// +WSAGetLastError();
		swprintf(errorStr, 50, L"소켓 초기화 실패%s", WSAGetLastError());
		MessageBox(nullptr, errorStr, L"알림", MB_OK);
		ASSERT(false);
		return false;
	}
	std::cout << "소켓 초기화 성공 " << std::endl;


	return true;
}

bool IOCompletionPort::BindandListen(int nBindPort)
{
	SOCKADDR_IN stServerAddr;
	stServerAddr.sin_family = AF_INET;
	stServerAddr.sin_port = htons(nBindPort);
	stServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	int nRet = bind(mListenSocket, (SOCKADDR*)&stServerAddr, sizeof(SOCKADDR_IN));
	if (nRet != 0)
	{
		wchar_t errorStr[64];// +WSAGetLastError();
		swprintf(errorStr, 50, L"bind 실패 %s", WSAGetLastError());
		MessageBox(nullptr, errorStr, L"알림", MB_OK);
		ASSERT(false);
		return false;
	}

	nRet = listen(mListenSocket, SOMAXCONN);
	if (nRet != 0)
	{
		wchar_t errorStr[64];// +WSAGetLastError();
		swprintf(errorStr, 50, L"listen 실패 %s", WSAGetLastError());
		MessageBox(nullptr, errorStr, L"알림", MB_OK);
		ASSERT(false);
		return false;
	}

	std::cout << "listen 성공 " << std::endl;
	return true;
}

bool IOCompletionPort::StartServer(const UINT32 maxClientCount)
{
	createClient(maxClientCount);

	mIOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, MAX_WORKERTHREAD);
	if (mIOCPHandle == nullptr)
	{
		wchar_t errorStr[64];// +WSAGetLastError();
		swprintf(errorStr, 50, L"CreateIoCompletionPort 실패 %s", WSAGetLastError());
		MessageBox(nullptr, errorStr, L"알림", MB_OK);
		ASSERT(false);
		return false;
	}

	bool bRet = createIOWorkerThread();
	if (!bRet)
	{
		ASSERT(false);
		return false;
	}

	bRet = createAcceptorThread();
	if (!bRet)
	{
		ASSERT(false);
		return false;
	}
	std::cout << "Server Start" << std::endl;
}

void IOCompletionPort::DestroyThread()
{
	mIsWorkerRun = false;
	CloseHandle(mIOCPHandle);

	for (auto& th : mIOWorkerThreads)
	{
		if (th.joinable())
		{
			th.join();
		}
	}

	mIsAccepterRun = false;
	closesocket(mListenSocket);

	if (mAccepterThread.joinable())
	{
		mAccepterThread.join();
	}
}

stClientInfo* IOCompletionPort::getEmptyClientInfoOrNull()
{
	for (stClientInfo& client : mClientInfos)
	{
		if (INVALID_SOCKET == client.m_socketClient)
		{
			return &client;
		}
	}
	return nullptr;
}

bool IOCompletionPort::bindIOCompletionPort(stClientInfo* pClientInfo)
{
	ASSERT(pClientInfo->m_socketClient != NULL);
	ASSERT(pClientInfo->m_socketClient != INVALID_SOCKET);

	HANDLE hIOCP = CreateIoCompletionPort((HANDLE)pClientInfo->m_socketClient, mIOCPHandle, (ULONG_PTR)pClientInfo, 0);

	if (hIOCP == nullptr || mIOCPHandle != hIOCP)
	{
		ASSERT(false);
		return false;
	}
	return true;
}

void IOCompletionPort::ioworkerthread()
{
	stClientInfo* pClientInfo = NULL;

	BOOL isSuccess = true;

	DWORD dwIOSize = 0;
	LPOVERLAPPED lpOverlapped = NULL;

	while (mIsWorkerRun)
	{
		isSuccess = GetQueuedCompletionStatus(
			mIOCPHandle,
			&dwIOSize,
			(PULONG_PTR)&pClientInfo,
			&lpOverlapped,
			INFINITE
		);

		//사용자 쓰레드 종료 메세지 처리..
		if (true == isSuccess && 0 == dwIOSize && NULL == lpOverlapped)
		{
			mIsWorkerRun = false;
			continue;
		}

		if (NULL == lpOverlapped)
		{
			ASSERT(false);
			continue;
		}

		if (false == isSuccess || (0 == dwIOSize && true == isSuccess))
		{
			std::cout << "Socket(" << (int)pClientInfo->m_socketClient << ") 접속 끊김" << std::endl;
			closeSocket(pClientInfo);
			continue;
		}

		stOverlappedEx* pOverlappedEx = (stOverlappedEx*)lpOverlapped;
		ASSERT((pOverlappedEx->m_eOperation == IOOperation::RECV) || (pOverlappedEx->m_eOperation == IOOperation::SEND));

		if (IOOperation::RECV == pOverlappedEx->m_eOperation)
		{
			pOverlappedEx->m_szBuf[dwIOSize] = NULL;
			printf("[수신] bytes : %d, msg %s\n", dwIOSize, pOverlappedEx->m_szBuf);

			//Echo
			bool isSendSuccess = sendMsg(pClientInfo, pOverlappedEx->m_szBuf, dwIOSize);
			ASSERT(isSendSuccess);
			//recv...
			beginRecv(pClientInfo);
		}

		else if (IOOperation::SEND == pOverlappedEx->m_eOperation)
		{
			printf("[송신] bytes : %d, msg : %s\n", dwIOSize, pOverlappedEx->m_szBuf);
		}
		else
		{
			ASSERT(false);
			printf("socket(%d)에서 예외상황\n", (int)pClientInfo->m_socketClient);
		}
	}
}

bool IOCompletionPort::sendMsg(stClientInfo* pClientInfo, char* pMsg, int nLen)
{
	DWORD dwRecvNumBytes = 0;

	CopyMemory(pClientInfo->m_stSendOverlappedEx.m_szBuf, pMsg, nLen);

	pClientInfo->m_stSendOverlappedEx.m_wsaBuf.len = nLen;
	pClientInfo->m_stSendOverlappedEx.m_wsaBuf.buf = pClientInfo->m_stSendOverlappedEx.m_szBuf;
	pClientInfo->m_stSendOverlappedEx.m_eOperation = IOOperation::SEND;

	int nRet = WSASend(
		pClientInfo->m_socketClient,
		&(pClientInfo->m_stSendOverlappedEx.m_wsaBuf),
		1,
		&dwRecvNumBytes,
		0,
		(LPWSAOVERLAPPED) & (pClientInfo->m_stSendOverlappedEx),
		NULL
	);
	if (nRet == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
	{
		ASSERT(false);
		wchar_t* s = NULL;
		FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, WSAGetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPWSTR)&s, 0, NULL);
		printf("[에러] WSASend()함수 실패 : %d %S\n", WSAGetLastError(), s);
		LocalFree(s);

		return false;
	}
	return true;
}

void IOCompletionPort::closeSocket(stClientInfo* pClientInfo, bool bIsForce)
{
	struct linger stlinger = { 0,0 };

	if (true == bIsForce)
	{
		stlinger.l_onoff = 1;
	}

	shutdown(pClientInfo->m_socketClient, SD_BOTH);

	setsockopt(pClientInfo->m_socketClient, SOL_SOCKET, SO_LINGER, (char*)&stlinger, sizeof(stlinger));

	closesocket(pClientInfo->m_socketClient);

	pClientInfo->m_socketClient = INVALID_SOCKET;
}

void IOCompletionPort::acceptorThread()
{
	SOCKADDR_IN stClientAddr;
	int nAddrLen = sizeof(SOCKADDR_IN);

	while (mIsAccepterRun)
	{
		stClientInfo* pClientInfo = getEmptyClientInfoOrNull();
		if (pClientInfo == nullptr)
		{
			std::cout << "Client Full" << std::endl;
			ASSERT(false);
			return;
		}
		//blocking
		pClientInfo->m_socketClient = accept(mListenSocket, (SOCKADDR*)&stClientAddr, &nAddrLen);
		if (INVALID_SOCKET == pClientInfo->m_socketClient)
		{
			continue;
		}

		bool bRet = bindIOCompletionPort(pClientInfo);
		if (!bRet)
		{
			DEBUGBREAK;
			return;
		}

		bRet = beginRecv(pClientInfo);
		if (!bRet)
		{
			return;
		}

		char clientIP[32] = { 0, };
		inet_ntop(AF_INET, &(stClientAddr.sin_addr), clientIP, 32 - 1);
		std::cout << "클라이언트 접속 : IP(" << clientIP << ") SOCKET(" << (int)pClientInfo->m_socketClient << ")" << std::endl;

	}
}

bool IOCompletionPort::beginRecv(stClientInfo* pClientInfo)
{
	DWORD dwFlag = 0;
	DWORD dwRecvNumBytes = 0;

	pClientInfo->m_stRecvOverlappedEx.m_wsaBuf.len = MAX_SOCKBUF;
	pClientInfo->m_stRecvOverlappedEx.m_wsaBuf.buf = pClientInfo->m_stRecvOverlappedEx.m_szBuf;
	pClientInfo->m_stRecvOverlappedEx.m_eOperation = IOOperation::RECV;

	int nRet = WSARecv(pClientInfo->m_socketClient,
		&(pClientInfo->m_stRecvOverlappedEx.m_wsaBuf),
		1,
		&dwRecvNumBytes,
		&dwFlag,
		(LPWSAOVERLAPPED) & (pClientInfo->m_stRecvOverlappedEx),
		NULL
	);

	if (nRet = SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
	{
		ASSERT(false);
		wchar_t* s = NULL;
		FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, WSAGetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPWSTR)&s, 0, NULL);
		printf("[에러] WSARecv() 함수 실패 : %d %S\n", WSAGetLastError(), s);
		LocalFree(s);
		return false;
	}

	return true;
}

void IOCompletionPort::createClient(const UINT32 maxClientCount)
{
	for (UINT32 i = 0; i < maxClientCount; ++i)
	{
		mClientInfos.emplace_back();
	}
}

bool IOCompletionPort::createIOWorkerThread()
{
	for (int i = 0; i < MAX_WORKERTHREAD; ++i)
	{
		mIOWorkerThreads.emplace_back([this]() {ioworkerthread(); });
	}
	std::cout << "IO Workerthread begin" << std::endl;
	return true;
}

bool IOCompletionPort::createAcceptorThread()
{
	mAccepterThread = std::thread([this]() {acceptorThread(); });

	std::cout << "AcceptorThread begin " << std::endl;
	return true;
}

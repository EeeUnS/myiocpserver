#include "pch.h"
#include "IOCompletionPort.hpp"
#include "DebugHelper.h"

CIOCompletionPort::~CIOCompletionPort()
{
	WSACleanup();
}

bool CIOCompletionPort::InitSocket()
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

bool CIOCompletionPort::BindandListen(int nBindPort)
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

bool CIOCompletionPort::StartServer(const UINT32 maxClientCount, int numIOThread)
{
	m_numIOThread = numIOThread;
	createClient(maxClientCount);

	m_hIOCPPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, MAX_WORKERTHREAD);
	if (m_hIOCPPort == nullptr)
	{
		wchar_t errorStr[64];// +WSAGetLastError();
		swprintf(errorStr, 50, L"CreateIoCompletionPort 실패 %s", WSAGetLastError());
		MessageBox(nullptr, errorStr, L"알림", MB_OK);
		ASSERT(false);
		return false;
	}

	for (unsigned int i = 0; i < m_numIOThread; ++i)
	{
		m_hIOThreads[i] = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, IOWorkerThread, this, 0, nullptr));
		SetThreadDescription(m_hIOThreads[i], L"IOOverlappedThread");
	}
	std::cout << "IO Workerthread begin" << std::endl;

	m_hAcceptThread = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, AcceptThread, this, 0, nullptr));
	SetThreadDescription(m_hAcceptThread, L"AcceptThread");

	std::cout << "Server Start" << std::endl;
}

void CIOCompletionPort::DestroyThread()
{
	mIsWorkerRun = false;

	for (unsigned int i = 0; i < m_numIOThread; ++i)
	{
		PostQueuedCompletionStatus(m_hIOCPPort, 0, 0, nullptr);
	}
	WaitForMultipleObjects(m_numIOThread, m_hIOThreads, true, INFINITE);
	WaitForSingleObject(m_hAcceptThread, INFINITE);
	

	for (unsigned int i = 0; i < m_numIOThread; ++i)
	{
		CloseHandle(m_hIOThreads[i]);

		m_hIOThreads[i] = INVALID_HANDLE_VALUE;
	}

	CloseHandle(m_hAcceptThread);
	m_hAcceptThread = INVALID_HANDLE_VALUE;

	CloseHandle(m_hIOCPPort);
	m_hIOCPPort = INVALID_HANDLE_VALUE;

	mIsAccepterRun = false;
	closesocket(mListenSocket);

}

stClientInfo* CIOCompletionPort::getEmptyClientInfoOrNull()
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

bool CIOCompletionPort::bindIOCompletionPort(stClientInfo* pClientInfo)
{
	ASSERT(pClientInfo->m_socketClient != NULL);
	ASSERT(pClientInfo->m_socketClient != INVALID_SOCKET);

	HANDLE hIOCP = CreateIoCompletionPort((HANDLE)pClientInfo->m_socketClient, m_hIOCPPort, (ULONG_PTR)pClientInfo, 0);

	if (hIOCP == nullptr || m_hIOCPPort != hIOCP)
	{
		ASSERT(false);
		return false;
	}
	return true;
}

unsigned int __stdcall CIOCompletionPort::IOWorkerThread(void* param)
{
	CIOCompletionPort* pThis = (CIOCompletionPort*)param;
	stClientInfo* pClientInfo = NULL;

	bool isSuccess = true;

	DWORD dwIOSize = 0;
	LPOVERLAPPED lpOverlapped = NULL;

	while (pThis->mIsWorkerRun)
	{
		isSuccess = GetQueuedCompletionStatus(
			pThis->m_hIOCPPort,
			&dwIOSize,
			(PULONG_PTR)&pClientInfo,
			&lpOverlapped,
			INFINITE
		);

		//사용자 쓰레드 종료 메세지 처리..
		if (true == isSuccess && 0 == dwIOSize && NULL == lpOverlapped)
		{
			pThis->mIsWorkerRun = false;
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
			pThis->closeSocket(pClientInfo);
			continue;
		}

		stOverlappedEx* pOverlappedEx = (stOverlappedEx*)lpOverlapped;
		ASSERT((pOverlappedEx->m_eOperation == eIOOperation::RECV) || (pOverlappedEx->m_eOperation == eIOOperation::SEND));

		ASSERT((pOverlappedEx->m_eOperation == eIOOperation::RECV) && (&pClientInfo->m_stRecvOverlappedEx == pOverlappedEx)
			|| (pOverlappedEx->m_eOperation == eIOOperation::SEND) && (&pClientInfo->m_stSendOverlappedEx == pOverlappedEx)
		);


		if (eIOOperation::RECV == pOverlappedEx->m_eOperation)
		{
			pClientInfo->m_szRecvBuf[dwIOSize] = NULL;
			printf("[수신] bytes : %d, msg %s\n", dwIOSize, pClientInfo->m_szRecvBuf);

			//Echo
			bool isSendSuccess = pThis->sendMsg(pClientInfo, pClientInfo->m_szRecvBuf, dwIOSize);
			ASSERT(isSendSuccess);
			//recv...
			pThis->beginRecv(pClientInfo);
		}

		else if (eIOOperation::SEND == pOverlappedEx->m_eOperation)
		{
			printf("[송신] bytes : %d, msg : %s\n", dwIOSize, pClientInfo->m_szSendBuf);
		}
		else
		{
			ASSERT(false);
			printf("socket(%d)에서 예외상황\n", (int)pClientInfo->m_socketClient);
		}
	}
	return 0;
}

bool CIOCompletionPort::sendMsg(stClientInfo* pClientInfo, char* pMsg, int nLen)
{
	DWORD dwRecvNumBytes = 0;

	ASSERT(strlen(pClientInfo->m_szSendBuf) >= nLen);
	ASSERT(strlen(pMsg) >= nLen);

	CopyMemory(pClientInfo->m_szSendBuf, pMsg, nLen);

	pClientInfo->m_stSendOverlappedEx.m_wsaBuf.len = nLen;
	pClientInfo->m_stSendOverlappedEx.m_wsaBuf.buf = pClientInfo->m_szSendBuf;
	pClientInfo->m_stSendOverlappedEx.m_eOperation = eIOOperation::SEND;

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

void CIOCompletionPort::closeSocket(stClientInfo* pClientInfo, bool bIsForce)
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

//[ThreadOnly]
unsigned int __stdcall   CIOCompletionPort::AcceptThread(void* param)
{
	CIOCompletionPort* pThis = (CIOCompletionPort*)param;
	SOCKADDR_IN stClientAddr;
	int nAddrLen = sizeof(SOCKADDR_IN);

	while (pThis->mIsAccepterRun)
	{
		stClientInfo* pClientInfo = pThis->getEmptyClientInfoOrNull();
		if (pClientInfo == nullptr)
		{
			std::cout << "Client Full" << std::endl;
			ASSERT(false);
			return -1;
		}
		//blocking
		pClientInfo->m_socketClient = accept(pThis->mListenSocket, (SOCKADDR*)&stClientAddr, &nAddrLen);
		if (INVALID_SOCKET == pClientInfo->m_socketClient)
		{
			continue;
		}

		bool bRet = pThis->bindIOCompletionPort(pClientInfo);
		if (!bRet)
		{
			DEBUGBREAK;
			return -1;
		}

		bRet = pThis->beginRecv(pClientInfo);
		if (!bRet)
		{
			return -1;
		}

		char clientIP[32] = { 0, };
		inet_ntop(AF_INET, &(stClientAddr.sin_addr), clientIP, 32 - 1);
		std::cout << "클라이언트 접속 : IP(" << clientIP << ") SOCKET(" << (int)pClientInfo->m_socketClient << ")" << std::endl;

	}
	return 0;
}

bool CIOCompletionPort::beginRecv(stClientInfo* pClientInfo)
{
	DWORD dwFlag = 0;
	DWORD dwRecvNumBytes = 0;

	pClientInfo->m_stRecvOverlappedEx.m_wsaBuf.len = MAX_SOCKBUF;
	pClientInfo->m_stRecvOverlappedEx.m_wsaBuf.buf = pClientInfo->m_szRecvBuf;
	pClientInfo->m_stRecvOverlappedEx.m_eOperation = eIOOperation::RECV;

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

void CIOCompletionPort::createClient(const UINT32 maxClientCount)
{
	for (UINT32 i = 0; i < maxClientCount; ++i)
	{
		mClientInfos.emplace_back();
	}
}

//bool IOCompletionPort::createIOWorkerThread()
//{
//	return true;
//}
//
//bool IOCompletionPort::createAcceptorThread()
//{
//	mAccepterThread = std::thread([this]() {acceptorThread(); });
//
//	std::cout << "AcceptorThread begin " << std::endl;
//	return true;
//}

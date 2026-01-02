#pragma once
#pragma comment (lib, "ws2_32")

#include <WinSock2.h>
#include <Ws2tcpip.h>
#include "Session.h"

enum { MAX_WORKERTHREAD = 4 };


class CIOCompletionPort
{
public:
	CIOCompletionPort() = default;
	~CIOCompletionPort();

	bool InitSocket();

	bool BindandListen(int nBindPort);

	bool StartServer(const UINT32 maxClientCount, int numIOThread);


	void DestroyThread();


private:
	SOCKET mListenSocket;
	std::vector<CSession> mClientInfos;

	HANDLE m_hIOCPPort;
	HANDLE m_hAcceptThread;
	HANDLE m_hIOThreads[MAX_WORKERTHREAD];
	int m_numIOThread;

	bool mIsWorkerRun = true;
	bool mIsAccepterRun = true;

	CSession* getEmptyClientInfoOrNull();

	bool sendMsg(CSession* pClientInfo, char* pMsg, int nLen);

	void closeSocket(CSession* pClientInfo, bool bIsForce = false);

	// Thread Function
	static unsigned int __stdcall IOWorkerThread(void* param);
	static unsigned int __stdcall AcceptThread(void* param);

	bool beginRecv(CSession* pClientInfo);

	void createClient(const UINT32 maxClientCount);

	CACHE_LINE unsigned int		mCurrentSessionCount;
	
	//CACHE_LINE unsigned int		mNumAccept;
	//CACHE_LINE unsigned int		mNumRecv;
	//CACHE_LINE unsigned int		mNumSend;
	//CACHE_LINE unsigned int		mMaximumSessionCount;
	//CACHE_LINE unsigned int		mNumRequestDisconnected;
	//CACHE_LINE unsigned int		mNumSendQueueFullDisconnected;
	//CACHE_LINE unsigned int		mNumInvalidSessionDisconnected;
	//CACHE_LINE unsigned int		mNumLimitSessionCountDisconnected;
};
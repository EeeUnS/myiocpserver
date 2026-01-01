#pragma once
#pragma comment (lib, "ws2_32")

#include <WinSock2.h>
#include <Ws2tcpip.h>

enum { MAX_SOCKBUF = _4k }; // 4kB
enum { MAX_WORKERTHREAD = 4 };

enum class eIOOperation
{
	NONE = 0,
	RECV,
	SEND
};

struct stOverlappedEx
{
	WSAOVERLAPPED m_wsaOverlapped;
	SOCKET m_socketClient;
	WSABUF m_wsaBuf;
	eIOOperation m_eOperation;
};

struct stClientInfo
{
	SOCKET m_socketClient;
	stOverlappedEx m_stRecvOverlappedEx;
	stOverlappedEx m_stSendOverlappedEx;
	char m_szSendBuf[MAX_SOCKBUF];
	char m_szRecvBuf[MAX_SOCKBUF];

	stClientInfo()
	{
		ZeroMemory(&m_stRecvOverlappedEx, sizeof(stOverlappedEx));
		ZeroMemory(&m_stSendOverlappedEx, sizeof(stOverlappedEx));
		m_socketClient = INVALID_SOCKET;
	}
};


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
	std::vector<stClientInfo> mClientInfos;

	HANDLE m_hIOCPPort;
	HANDLE m_hAcceptThread;
	HANDLE m_hIOThreads[MAX_WORKERTHREAD];
	int m_numIOThread;

	bool mIsWorkerRun = true;
	bool mIsAccepterRun = true;

	stClientInfo* getEmptyClientInfoOrNull();

	bool bindIOCompletionPort(stClientInfo* pClientInfo);


	bool sendMsg(stClientInfo* pClientInfo, char* pMsg, int nLen);

	void closeSocket(stClientInfo* pClientInfo, bool bIsForce = false);

	// Thread Function
	static unsigned int __stdcall IOWorkerThread(void* param);
	static unsigned int __stdcall AcceptThread(void* param);

	bool beginRecv(stClientInfo* pClientInfo);

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
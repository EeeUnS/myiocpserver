#pragma once

enum { MAX_SOCKBUF = _4k }; // 4kB
enum class eIOOperation
{
	NONE = 0,
	RECV,
	SEND
};

struct CSession
{
	struct stOverlappedEx
	{
		WSAOVERLAPPED m_wsaOverlapped;
		SOCKET m_socketClient;
		WSABUF m_wsaBuf;
		eIOOperation m_eOperation;
	};

	SOCKET m_socketClient;
	stOverlappedEx m_stRecvOverlappedEx;
	stOverlappedEx m_stSendOverlappedEx;
	char m_szSendBuf[MAX_SOCKBUF];
	char m_szRecvBuf[MAX_SOCKBUF];

	int m_nPort;
	unsigned long m_nIp;

	CSession() : m_nPort(0), m_nIp(0)
	{
		ZeroMemory(&m_stRecvOverlappedEx, sizeof(stOverlappedEx));
		ZeroMemory(&m_stSendOverlappedEx, sizeof(stOverlappedEx));
		m_socketClient = INVALID_SOCKET;
	}
};

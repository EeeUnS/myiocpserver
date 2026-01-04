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
	struct COverlappedEx
	{
		WSAOVERLAPPED m_wsaOverlapped;
		SOCKET m_socketClient;
		WSABUF m_wsaBuf;
		eIOOperation m_eOperation;
	};
	static_assert(offsetof(COverlappedEx, m_wsaOverlapped) == 0, "COverlappedEx size is incorrect");

	SOCKET m_socketClient;
	COverlappedEx m_RecvOverlappedEx;
	COverlappedEx m_stSendOverlappedEx;

	char m_szSendBuf[MAX_SOCKBUF];
	char m_szRecvBuf[MAX_SOCKBUF];

	int m_nPort;
	unsigned long m_nIp;

	CSession() : m_nPort(0), m_nIp(0)
	{
		ZeroMemory(&m_RecvOverlappedEx, sizeof(COverlappedEx));
		ZeroMemory(&m_stSendOverlappedEx, sizeof(COverlappedEx));
		m_socketClient = INVALID_SOCKET;
	}
};

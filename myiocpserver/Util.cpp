#include "pch.h"
#include "Util.h"

namespace Util
{
	bool TryGetProcessName(TCHAR processName[MAX_PATH])
	{
		TCHAR processPath[MAX_PATH] = { 0 };
		DWORD error = GetModuleFileName(NULL, processPath, MAX_PATH);
		if (error == 0 || error == MAX_PATH)
		{
			lstrcpy(processName, TEXT("UnknownProcess"));
			return false;
		}

		// Extract just the filename (without path)
		TCHAR* pProcessName = processPath;
		for (TCHAR* p = processPath; *p; ++p)
		{
			if (*p == '\\' || *p == '/')
				pProcessName = p + 1;
		}

		lstrcpy(processName, pProcessName);
		return true;
	}

	const char* WSAError(int errorcode)
	{
		switch (errorcode)
		{
		case WSA_INVALID_HANDLE: // 6
			return "지정된 이벤트 개체 핸들이 잘못되었습니다. 애플리케이션이 이벤트 개체를 사용하려고 하지만 지정된 핸들이 잘못되었습니다.";
		case WSA_NOT_ENOUGH_MEMORY: // 8
			return "사용 가능한 메모리가 부족합니다. 애플리케이션은 Windows 함수에 직접 매핑되는 Windows 소켓 함수를 사용했습니다. Windows 함수는 필요한 메모리 리소스가 없음을 나타냅니다.";
		case WSA_INVALID_PARAMETER: // 87
			return "하나 이상의 매개 변수가 잘못되었습니다. 애플리케이션은 Windows 함수에 직접 매핑되는 Windows 소켓 함수를 사용했습니다. Windows 함수는 하나 이상의 매개 변수에 문제가 있음을 나타냅니다.";
		case WSA_OPERATION_ABORTED: // 995
			return "겹치는 작업이 중단되었습니다. 소켓의 닫기 또는 WSAIoctl에서 SIO_FLUSH 명령 실행으로 인해 겹치는 작업이 취소되었습니다.";
		case WSA_IO_INCOMPLETE: // 996
			return "겹치는 I/O 이벤트 개체가 신호 상태가 아닙니다. 애플리케이션은 아직 완료되지 않은 겹치는 작업의 상태를 확인하려고 했습니다. 폴링 모드에서 WSAGetOverlappedResult(fWait 플래그가 FALSE로 설정됨)를 사용하여 겹치는 작업이 완료된 시기를 결정하는 애플리케이션은 작업이 완료될 때까지 이 오류 코드를 가져옵니다.";
		case WSA_IO_PENDING: // 997
			return "겹치는 작업은 나중에 완료됩니다. 애플리케이션에서 즉시 완료할 수 없는 겹쳐진 작업을 시작했습니다. 작업이 완료되면 나중에 완료 표시가 제공됩니다.";
		case WSAEINTR: // 10004
			return "함수 호출이 중단되었습니다. WSACancelBlockingCall 호출로 인해 차단 작업이 중단되었습니다.";
		case WSAEBADF: // 10009
			return "파일 핸들이 잘못되었습니다. 제공된 파일 핸들이 잘못되었습니다.";
		case WSAEACCES: // 10013
			return "사용 권한이 거부되었습니다. 액세스 권한에 의해 금지된 방식으로 소켓에 액세스하려고 시도했습니다. 예를 들어 setsockopt(SO_BROADCAST)를 사용하여 브로드캐스트 권한을 설정하지 않고 sendto에 대한 브로드캐스트 주소를 사용하는 것입니다. WSAEACCES 오류의 또 다른 가능한 이유는 바인딩 함수가 호출될 때(SP4 이상에서 Windows NT 4.0에서) 다른 애플리케이션, 서비스 또는 커널 모드 드라이버가 단독 액세스 권한이 있는 동일한 주소에 바인딩된다는 것입니다. 이러한 단독 액세스는 SP4 이상에서 Windows NT 4.0의 새로운 기능이며 SO_EXCLUSIVEADDRUSE 옵션을 사용하여 구현됩니다.";
		case WSAEFAULT: // 10014
			return "잘못된 주소입니다. 시스템에서 호출의 포인터 인수를 사용하려고 할 때 잘못된 포인터 주소를 검색했습니다. 이 오류는 애플리케이션이 잘못된 포인터 값을 전달하거나 버퍼 길이가 너무 작은 경우에 발생합니다. 예를 들어 sockaddr 구조체인 인수의 길이가 sizeof(sockaddr)보다 작습니다.";
		case WSAEINVAL: // 10022
			return "인수가 잘못되었습니다. 일부 잘못된 인수가 제공되었습니다(예: setsockopt 함수에 잘못된 수준 지정). 경우에 따라 수신 대기하지 않는 소켓에서 수락을 호출하는 경우 소켓의 현재 상태를 나타냅니다.";
		case WSAEMFILE: // 10024
			return "열려 있는 파일이 너무 많습니다. 열린 소켓이 너무 많습니다. 각 구현에는 전역적으로, 프로세스당 또는 스레드당 사용 가능한 최대 소켓 핸들 수가 있을 수 있습니다.";
		case WSAEWOULDBLOCK: // 10035
			return "리소스를 일시적으로 사용할 수 없습니다. 이 오류는 즉시 완료할 수 없는 비블로킹 소켓 작업(예: 소켓에서 읽을 데이터가 큐에 대기되지 않은 경우)에서 반환됩니다. 심각하지 않은 오류이며 나중에 작업을 다시 시도해야 합니다. 연결이 설정되려면 시간이 경과해야 하므로 WSAEWOULDBLOCK이 비블로킹 SOCK_STREAM 소켓에서 연결을 호출한 결과로 보고되는 것이 정상입니다.";
		case WSAEINPROGRESS: // 10036
			return "작업이 진행 중입니다. 차단 작업이 진행 중입니다. Windows 소켓은 단일 차단 작업(작업 또는 스레드당)만 미해결되도록 허용하며, 다른 함수 호출이 수행되는 경우(해당 소켓 또는 다른 소켓을 참조하는지 여부에 관계없이) WSAEINPROGRESS 오류로 인해 함수가 실패합니다.";
		case WSAEALREADY: // 10037
			return "작업이 이미 진행 중입니다. 작업이 이미 진행 중인 비차단 소켓에서 작업이 시도되었습니다. 즉, 이미 연결 중인 비차단 소켓에서 두 번째로 연결을 호출하거나 이미 취소되거나 완료된 비동기 요청(WSAAsyncGetXbyY)을 취소합니다.";
		case WSAENOTSOCK: // 10038
			return "nonsocket에 대한 소켓 작업입니다. 소켓이 아닌 항목에서 작업을 시도했습니다. 소켓 핸들 매개 변수가 유효한 소켓을 참조하지 않았거나 선택 항목의 경우 fd_set 멤버가 잘못되었습니다.";
		case WSAEDESTADDRREQ: // 10039
			return "대상 주소가 필요합니다. 소켓 작업에서 필수 주소가 생략되었습니다. 예를 들어 이 오류는 sendto가 ADDR_ANY 원격 주소로 호출되는 경우 반환됩니다.";
		case WSAEMSGSIZE: // 10040
			return "메시지가 너무 깁니다. 데이터그램 소켓에서 보낸 메시지가 내부 메시지 버퍼 또는 다른 네트워크 제한보다 크거나 데이터그램을 수신하는 데 사용되는 버퍼가 데이터그램 자체보다 작습니다.";
		case WSAEPROTOTYPE: // 10041
			return "소켓의 프로토콜 유형이 잘못되었습니다. 프로토콜은 요청된 소켓 유형의 의미 체계를 지원하지 않는 소켓 함수 호출에서 지정되었습니다. 예를 들어 ARPA 인터넷 UDP 프로토콜은 소켓 형식의 SOCK_STREAM을 지정할 수 없습니다.";
		case WSAENOPROTOOPT: // 10042
			return "잘못된 프로토콜 옵션입니다. 알 수 없거나 유효하지 않거나 지원되지 않는 옵션 또는 수준이 getsockopt 또는 setsockopt 호출에 지정되었습니다.";
		case WSAEPROTONOSUPPORT: // 10043
			return "프로토콜이 지원되지 않습니다. 요청된 프로토콜이 시스템에 구성되지 않았거나 이에 대한 구현이 없습니다. 예를 들어 소켓 호출은 SOCK_DGRAM 소켓을 요청하지만 스트림 프로토콜을 지정합니다.";
		case WSAESOCKTNOSUPPORT: // 10044
			return "소켓 유형은 지원되지 않습니다. 이 주소 패밀리에서는 지정된 소켓 형식이 지원되지 않습니다. 예를 들어 선택적 형식 SOCK_RAW 소켓 호출에서 선택될 수 있으며 구현은 SOCK_RAW 소켓을 전혀 지원하지 않습니다.";
		case WSAEOPNOTSUPP: // 10045
			return "작업이 지원되지 않습니다. 시도한 작업은 참조된 개체 형식에 대해 지원되지 않습니다. 일반적으로 이 작업은 이 작업을 지원할 수 없는 소켓에 대한 소켓 설명자가 데이터그램 소켓의 연결을 수락하려고 할 때 발생합니다.";
		case WSAEPFNOSUPPORT: // 10046
			return "프로토콜 패밀리는 지원되지 않습니다. 프로토콜 제품군이 시스템에 구성되지 않았거나 이에 대한 구현이 없습니다. 이 메시지는 WSAEAFNOSUPPORT와 약간 다른 의미를 줍니다. 그러나 대부분의 경우 서로 교환할 수 있으며 이러한 메시지 중 하나를 반환하는 모든 Windows 소켓 함수는 WSAEAFNOSUPPORT도 지정합니다.";
		case WSAEAFNOSUPPORT: // 10047
			return "프로토콜 패밀리에서 지원되지 않는 주소 패밀리입니다. 요청한 프로토콜과 호환되지 않는 주소가 사용되었습니다. 모든 소켓은 연결된 주소 패밀리(즉, 인터넷 프로토콜의 경우 AF_INET) 및 일반 프로토콜 유형(즉, SOCK_STREAM)을 사용하여 만들어집니다. 이 오류는 소켓 호출에서 잘못된 프로토콜이 명시적으로 요청되거나 잘못된 패밀리의 주소가 소켓(예: sendto)에 사용되는 경우 반환됩니다.";
		case WSAEADDRINUSE: // 10048
			return "이미 사용 중인 주소입니다. 일반적으로 각 소켓 주소(프로토콜/IP 주소/포트)의 사용은 하나만 허용됩니다. 이 오류는 애플리케이션이 기존 소켓에 이미 사용된 IP 주소/포트 또는 제대로 닫히지 않은 소켓 또는 아직 닫고 있는 소켓에 소켓을 바인딩 하려고 시도하는 경우에 발생합니다. 여러 소켓을 동일한 포트 번호에 바인딩 해야 하는 서버 애플리케이션의 경우 setsockopt(SO_REUSEADDR)를 사용하는 것이 좋습니다. 클라이언트 애플리케이션은 일반적으로 바인딩을 전혀 호출할 필요가 없습니다. 연결은 사용하지 않는 포트를 자동으로 선택합니다. 와일드카드 주소(ADDR_ANY 포함)를 사용하여 바인딩을 호출하면 특정 주소가 커밋될 때까지 WSAEADDRINUSE 오류가 지연될 수 있습니다. 나중에 연결, 수신 대기, WSAConnect 또는 WSAJoinLeaf를 비롯한 다른 함수를 호출할 때 발생할 수 있습니다.";
		case WSAEADDRNOTAVAIL: // 10049
			return "요청된 주소를 할당할 수 없습니다. 요청한 주소가 해당 컨텍스트에서 잘못되지 않음. 이는 일반적으로 로컬 컴퓨터에 유효하지 않은 주소에 바인딩 하려는 시도에서 발생합니다. 원격 주소 또는 포트가 원격 컴퓨터(예: 주소 또는 포트 0)에 유효하지 않은 경우 연결, sendto, WSAConnect, WSAJoinLeaf 또는 WSASendTo에서 발생할 수도 있습니다.";
		case WSAENETDOWN: // 10050
			return "네트워크가 다운되었습니다. 소켓 작업에서 작동하지 않는 네트워크가 검색되었습니다. 네트워크 시스템(Windows Sockets.dll이 작동하는 프로토콜 스택), 네트워크 인터페이스 또는 로컬 네트워크에 문제가 발생했을 수 있습니다.";
		case WSAENETUNREACH: // 10051
			return "네트워크에 연결할 수 없습니다. 연결할 수 없는 네트워크에서 소켓 작업을 시도했습니다. 이는 일반적으로 로컬 소프트웨어가 원격 호스트에 연결할 경로를 알 수 없음을 의미합니다.";
		case WSAENETRESET: // 10052
			return "다시 설정할 때 네트워크 연결이 끊어지게 됩니다. 해당 작업이 진행되는 동안 오류가 발생하여 연결이 끊겼습니다. 이미 실패한 연결에서 SO_KEEPALIVE 설정하려고 시도하는 경우 setsockopt에서 반환할 수도 있습니다.";
		case WSAECONNABORTED: // 10053
			return "소프트웨어로 인해 연결이 중단되었습니다. 데이터 전송 시간 제한 또는 프로토콜 오류로 인해 호스트 컴퓨터의 소프트웨어에 의해 설정된 연결이 중단되었습니다.";
		case WSAECONNRESET: // 10054
			return "피어별 연결 다시 설정. 현재 연결은 원격 호스트에 의해 강제로 끊겼습니다. 일반적으로 원격 호스트의 피어 애플리케이션이 갑자기 중지되거나, 호스트가 다시 부팅되거나, 호스트 또는 원격 네트워크 인터페이스가 비활성화되거나, 원격 호스트가 하드 닫기를 사용하는 경우 발생합니다(원격 소켓의 SO_LINGER 옵션에 대한 자세한 내용은 setsockopt 참조). 이 오류는 하나 이상의 작업이 진행 중인 동안 실패를 감지하는 연결 유지 활동으로 인해 연결이 끊어진 경우에도 발생할 수 있습니다. 진행 중인 작업이 WSAENETRESET으로 실패합니다. WSAECONNRESET에서 후속 작업이 실패합니다.";
		case WSAENOBUFS: // 10055
			return "사용할 수 있는 버퍼 공간이 없습니다. 시스템에 충분한 버퍼 공간이 부족하거나 큐가 가득 차서 소켓에서 작업을 수행할 수 없습니다.";
		case WSAEISCONN: // 10056
			return "소켓이 이미 연결되어 있습니다. 이미 연결된 소켓에서 연결 요청이 이루어졌습니다. 또한 일부 구현에서는 sendto가 연결된 SOCK_DGRAM 소켓에서 호출되는 경우에도 이 오류를 반환합니다(SOCK_STREAM 소켓의 경우 sendto의 to 매개 변수는 무시됨).";
		case WSAENOTCONN: // 10057
			return "소켓이 연결되어 있지 않습니다. 소켓이 연결되지 않았고(sendto를 사용하여 데이터그램 소켓을 보낼 때) 주소가 제공되지 않았기 때문에 데이터를 보내거나 받는 요청이 허용되지 않았습니다. 다른 유형의 작업도 이 오류를 반환할 수 있습니다. 예를 들어 연결이 다시 설정되면 setsockopt 설정 SO_KEEPALIVE.";
		case WSAESHUTDOWN: // 10058
			return "소켓 종료 후에는 보낼 수 없습니다. 소켓이 이전 종료 호출을 통해 해당 방향으로 이미 종료되었기 때문에 데이터를 보내거나 받는 요청이 허용되지 않았습니다. 종료를 호출하여 소켓의 부분 닫기를 요청합니다. 이는 송신 또는 수신 중이거나 둘 다 중단되었다는 신호입니다.";
		case WSAETOOMANYREFS: // 10059
			return "참조가 너무 많습니다. 일부 커널 개체에 대한 참조가 너무 많습니다.";
		case WSAETIMEDOUT: // 10060
			return "연결 시간이 초과되었습니다. 연결된 당사자가 일정 기간 후에 제대로 응답하지 않았거나 연결된 호스트가 응답하지 않아 설정된 연결이 실패했기 때문에 연결 시도가 실패했습니다.";
		case WSAECONNREFUSED: // 10061
			return "연결이 거부되었습니다. 대상 컴퓨터가 적극적으로 거부했기 때문에 연결할 수 없습니다. 이는 일반적으로 서버 애플리케이션이 실행되지 않는 외부 호스트에서 비활성 상태인 서비스에 연결하려고 시도하기 때문에 발생합니다.";
		case WSAELOOP: // 10062
			return "이름을 번역할 수 없습니다. 이름을 번역할 수 없습니다.";
		case WSAENAMETOOLONG: // 10063
			return "이름이 너무 깁니다. 이름 구성 요소 또는 이름이 너무 깁니다.";
		case WSAEHOSTDOWN: // 10064
			return "호스트가 다운되었습니다. 대상 호스트가 다운되어 소켓 작업이 실패했습니다. 소켓 작업에서 호스트가 중지되었습니다. 로컬 호스트의 네트워킹 작업이 시작되지 않았습니다. 이러한 조건은 WSAETIMEDOUT 오류로 표시될 가능성이 높습니다.";
		case WSAEHOSTUNREACH: // 10065
			return "호스트할 경로가 없습니다. 연결할 수 없는 호스트로 소켓 작업을 시도했습니다. WSAENETUNREACH를 참조하세요.";
		case WSAENOTEMPTY: // 10066
			return "디렉터리가 비어 있지 않습니다. 비어 있지 않은 디렉터리는 제거할 수 없습니다.";
		case WSAEPROCLIM: // 10067
			return "프로세스가 너무 많습니다. Windows 소켓 구현은 동시에 사용할 수 있는 애플리케이션 수에 제한이 있을 수 있습니다. 제한에 도달한 경우 이 오류로 인해 WSAStartup이 실패할 수 있습니다.";
		case WSAEUSERS: // 10068
			return "사용자 할당량을 초과했습니다. 사용자 할당량이 부족합니다.";
		case WSAEDQUOT: // 10069
			return "디스크 할당량을 초과했습니다. 디스크 할당량이 부족합니다.";
		case WSAESTALE: // 10070
			return "부실 파일 핸들 참조입니다. 파일 핸들 참조를 더 이상 사용할 수 없습니다.";
		case WSAEREMOTE: // 10071
			return "항목이 원격입니다. 항목을 로컬로 사용할 수 없습니다.";
		case WSASYSNOTREADY: // 10091
			return "네트워크 하위 시스템을 사용할 수 없습니다. 이 오류는 네트워크 서비스를 제공하는 데 사용하는 기본 시스템을 현재 사용할 수 없으므로 현재 Windows 소켓 구현이 작동하지 않는 경우 WSAStartup에서 반환됩니다. 사용자는 적절한 Windows 소켓 DLL 파일이 현재 경로에 있는지 확인합니다. 둘 이상의 Windows 소켓 구현을 동시에 사용하려고 하지 않습니다. 시스템에 Winsock DLL이 두 개 이상 있는 경우 경로의 첫 번째 DLL이 현재 로드된 네트워크 하위 시스템에 적합한지 확인합니다. 필요한 모든 구성 요소가 현재 설치되고 올바르게 구성되어 있는지 확인하기 위한 Windows 소켓 구현 설명서입니다.";
		case WSAVERNOTSUPPORTED: // 10092
			return "Winsock.dll 버전이 범위를 벗어났습니다. 현재 Windows 소켓 구현은 애플리케이션에서 요청한 Windows 소켓 사양 버전을 지원하지 않습니다. 이전 Windows 소켓 DLL 파일에 액세스하고 있지 않은지 확인하십시오.";
		case WSANOTINITIALISED: // 10093
			return "WSAStartup이 아직 수행되지 않았습니다. 애플리케이션이 WSAStartup을 호출하지 않았거나 WSAStartup이 실패했습니다. 애플리케이션이 현재 활성 작업이 소유하지 않은 소켓(즉, 작업 간에 소켓을 공유하려고 시도)에 액세스하거나 WSACleanup이 너무 많이 호출되었을 수 있습니다.";
		case WSAEDISCON: // 10101
			return "정상 종료가 진행 중입니다. 원격 당사자가 정상적인 종료 시퀀스를 시작했음을 나타내기 위해 WSARecv 및 WSARecvFrom에서 반환됩니다.";
		case WSAENOMORE: // 10102
			return "더 이상 결과가 없습니다. WSALookupServiceNext 함수에서 더 이상 결과를 반환할 수 없습니다.";
		case WSAECANCELLED: // 10103
			return "통화가 취소되었습니다. 이 호출이 계속 처리되는 동안 WSALookupServiceEnd 함수에 대한 호출이 수행되었습니다. 통화가 취소되었습니다.";
		case WSAEINVALIDPROCTABLE: // 10104
			return "프로시저 호출 테이블이 잘못되었습니다. 서비스 공급자 프로시저 호출 테이블이 잘못되었습니다. 서비스 공급자가 Ws2_32.dll 위해 가짜 프로시저 테이블을 반환했습니다. 이는 일반적으로 하나 이상의 함수 포인터가 NULL이기 때문에 발생합니다.";
		case WSAEINVALIDPROVIDER: // 10105
			return "서비스 공급자가 잘못되었습니다. 요청한 서비스 공급자가 잘못되었습니다. 이 오류는 지정된 프로토콜 항목을 찾을 수 없는 경우 WSCGetProviderInfo 및 WSCGetProviderInfo32 함수에서 반환됩니다. 서비스 공급자가 2.0 이외의 버전 번호를 반환한 경우에도 이 오류가 반환됩니다.";
		case WSAEPROVIDERFAILEDINIT: // 10106
			return "서비스 공급자를 초기화하지 못했습니다. 요청한 서비스 공급자를 로드하거나 초기화할 수 없습니다. 이 오류는 서비스 공급자의 DLL을 로드할 수 없거나(LoadLibrary 실패) 공급자의 WSPStartup 또는 NSPStartup 함수가 실패한 경우 반환됩니다.";
		case WSASYSCALLFAILURE: // 10107
			return "시스템 호출 실패. 실패하지 않아야 하는 시스템 호출이 실패했습니다. 다양한 조건에서 반환되는 일반적인 오류 코드입니다. 실패하지 않아야 하는 시스템 호출이 실패할 때 반환됩니다. 예를 들어 WaitForMultipleEvents에 대한 호출이 실패하거나 레지스트리 함수 중 하나가 프로토콜/네임스페이스 카탈로그를 조작하려고 실패하는 경우입니다. 공급자가 SUCCESS를 반환하지 않고 확장된 오류 코드를 제공하지 않을 때 반환됩니다. 서비스 공급자 구현 오류를 나타낼 수 있습니다.";
		case WSASERVICE_NOT_FOUND: // 10108
			return "서비스를 찾을 수 없습니다. 알려진 서비스가 없습니다. 지정된 네임스페이스에서 서비스를 찾을 수 없습니다.";
		case WSATYPE_NOT_FOUND: // 10109
			return "클래스 형식을 찾을 수 없습니다. 지정된 클래스를 찾을 수 없습니다.";
		case WSA_E_NO_MORE: // 10110
			return "더 이상 결과가 없습니다. WSALookupServiceNext 함수에서 더 이상 결과를 반환할 수 없습니다.";
		case WSA_E_CANCELLED: // 10111
			return "통화가 취소되었습니다. 이 호출이 계속 처리되는 동안 WSALookupServiceEnd 함수에 대한 호출이 수행되었습니다. 통화가 취소되었습니다.";
		case WSAEREFUSED: // 10112
			return "데이터베이스 쿼리가 거부되었습니다. 적극적으로 거부되었기 때문에 데이터베이스 쿼리가 실패했습니다.";
		case WSAHOST_NOT_FOUND: // 11001
			return "호스트를 찾을 수 없습니다. 호스트를 확인할 수 없습니다. 이름은 공식 호스트 이름 또는 별칭이 아니거나 쿼리 중인 데이터베이스에서 찾을 수 없습니다. 이 오류는 프로토콜 및 서비스 쿼리에 대해 반환될 수도 있으며 지정된 이름을 관련 데이터베이스에서 찾을 수 없음을 의미합니다.";
		case WSATRY_AGAIN: // 11002
			return "인증되지 않은 호스트를 찾을 수 없습니다. 이는 일반적으로 호스트 이름 확인 중에 일시적인 오류이며 로컬 서버가 신뢰할 수 있는 서버로부터 응답을 받지 못했음을 의미합니다. 잠시 후 다시 시도하면 성공할 수 있습니다.";
		case WSANO_RECOVERY: // 11003
			return "복구할 수 없는 오류입니다. 이는 데이터베이스 조회 중에 일종의 복구할 수 없는 오류가 발생했음을 나타냅니다. 데이터베이스 파일(예: BSD 호환 HOSTS, SERVICES 또는 PROTOCOLS 파일)을 찾을 수 없거나 서버에서 심각한 오류로 DNS 요청을 반환했기 때문일 수 있습니다.";
		case WSANO_DATA: // 11004
			return "유효한 이름이며 요청된 형식의 데이터 레코드가 없습니다. 요청된 이름이 유효하고 데이터베이스에서 발견되었지만 올바른 연결된 데이터가 확인되지 않습니다. 이에 대한 일반적인 예는 DNS(도메인 이름 서버)를 사용하는 호스트 이름-주소 변환 시도(gethostbyname 또는 WSAAsyncGetHostByName 사용)입니다. MX 레코드가 반환되지만 호스트 자체가 존재하지만 직접 연결할 수 없음을 나타내는 A 레코드가 없습니다.";
		case WSA_QOS_RECEIVERS: // 11005
			return "QoS 수신기. 적어도 하나의 QoS 예약이 도착했습니다.";
		case WSA_QOS_SENDERS: // 11006
			return "QoS 보낸 사람. 하나 이상의 QoS 전송 경로가 도착했습니다.";
		case WSA_QOS_NO_SENDERS: // 11007
			return "QoS 보낸 사람이 없습니다. QoS 보낸 사람이 없습니다.";
		case WSA_QOS_NO_RECEIVERS: // 11008
			return "QoS 수신자가 없습니다. QoS 수신기가 없습니다.";
		case WSA_QOS_REQUEST_CONFIRMED: // 11009
			return "QoS 요청이 확인되었습니다. QoS 예약 요청이 확인되었습니다.";
		case WSA_QOS_ADMISSION_FAILURE: // 11010
			return "QoS 허용 오류입니다. 리소스 부족으로 인해 QoS 오류가 발생했습니다.";
		case WSA_QOS_POLICY_FAILURE: // 11011
			return "QoS 정책 실패. 정책 시스템에서 기존 정책 내에서 요청된 리소스를 할당할 수 없기 때문에 QoS 요청이 거부되었습니다.";
		case WSA_QOS_BAD_STYLE: // 11012
			return "QoS 잘못된 스타일입니다. 알 수 없거나 충돌하는 QoS 스타일이 발견되었습니다.";
		case WSA_QOS_BAD_OBJECT: // 11013
			return "QoS 잘못된 개체입니다. 일반적으로 filterspec 또는 공급자별 버퍼의 일부에서 문제가 발생했습니다.";
		case WSA_QOS_TRAFFIC_CTRL_ERROR: // 11014
			return "QoS 트래픽 제어 오류입니다. 제네릭 QoS 요청이 TC API에 의해 로컬 적용을 위해 변환되었기 때문에 기본 TC(트래픽 제어) API 오류가 발생했습니다. 메모리 부족 오류 또는 내부 QoS 공급자 오류 때문일 수 있습니다.";
		case WSA_QOS_GENERIC_ERROR: // 11015
			return "QoS 제네릭 오류입니다. 일반적인 QoS 오류입니다.";
		case WSA_QOS_ESERVICETYPE: // 11016
			return "QoS 서비스 유형 오류입니다. 잘못되었거나 인식할 수 없는 서비스 유형이 QoS flowspec에서 발견되었습니다.";
		case WSA_QOS_EFLOWSPEC: // 11017
			return "QoS flowspec 오류입니다. 잘못되었거나 일관되지 않은 flowspec이 QOS 구조에서 발견되었습니다.";
		case WSA_QOS_EPROVSPECBUF: // 11018
			return "QoS 공급자 버퍼가 잘못되었습니다. 잘못된 QoS 공급자별 버퍼입니다.";
		case WSA_QOS_EFILTERSTYLE: // 11019
			return "QoS 필터 스타일이 잘못되었습니다. 잘못된 QoS 필터 스타일이 사용되었습니다.";
		case WSA_QOS_EFILTERTYPE: // 11020
			return "QoS 필터 형식이 잘못되었습니다. 잘못된 QoS 필터 형식이 사용되었습니다.";
		case WSA_QOS_EFILTERCOUNT: // 11021
			return "잘못된 QoS 필터 수입니다. FLOWDESCRIPTOR에 잘못된 수의 QoS FILTERSPEC가 지정되었습니다.";
		case WSA_QOS_EOBJLENGTH: // 11022
			return "QoS 개체 길이가 잘못되었습니다. 잘못된 ObjectLength 필드가 있는 개체가 QoS 공급자별 버퍼에 지정되었습니다.";
		case WSA_QOS_EFLOWCOUNT: // 11023
			return "잘못된 QoS 흐름 수입니다. QoS 구조에 잘못된 수의 흐름 설명자가 지정되었습니다.";
		case WSA_QOS_EUNKOWNPSOBJ: // 11024
			return "인식할 수 없는 QoS 개체입니다. 인식할 수 없는 개체가 QoS 공급자별 버퍼에서 발견되었습니다.";
		case WSA_QOS_EPOLICYOBJ: // 11025
			return "잘못된 QoS 정책 개체입니다. 잘못된 정책 개체가 QoS 공급자별 버퍼에서 발견되었습니다.";
		case WSA_QOS_EFLOWDESC: // 11026
			return "잘못된 QoS 흐름 설명자입니다. 흐름 설명자 목록에서 잘못된 QoS 흐름 설명자를 찾았습니다.";
		case WSA_QOS_EPSFLOWSPEC: // 11027
			return "QoS 공급자별 flowspec이 잘못되었습니다. 잘못되었거나 일관되지 않은 flowspec이 QoS 공급자별 버퍼에서 발견되었습니다.";
		case WSA_QOS_EPSFILTERSPEC: // 11028
			return "QoS 공급자별 filterspec이 잘못되었습니다. 잘못된 FILTERSPEC가 QoS 공급자별 버퍼에서 발견되었습니다.";
		case WSA_QOS_ESDMODEOBJ: // 11029
			return "잘못된 QoS 셰이프 삭제 모드 개체입니다. 잘못된 셰이프 삭제 모드 개체가 QoS 공급자별 버퍼에서 발견되었습니다.";
		case WSA_QOS_ESHAPERATEOBJ: // 11030
			return "QoS 셰이핑 속도 개체가 잘못되었습니다. 잘못된 셰이핑 속도 개체가 QoS 공급자별 버퍼에서 발견되었습니다.";
		case WSA_QOS_RESERVED_PETYPE: // 11031
			return "예약된 정책 QoS 요소 형식입니다. 예약된 정책 요소가 QoS 공급자별 버퍼에서 발견되었습니다.";
		default:
			return "알 수 없는 WSA 오류입니다.";
		}
	}

	const char* WSARecvError(int errorcode)
	{
		switch (errorcode)
		{
		case WSAECONNABORTED: // 10053
			return "가상 회로가 시간 초과 또는 기타 오류로 인해 종료되었습니다.";
		case WSAECONNRESET: // 10054
			return "스트림 소켓의 경우 원격 쪽에서 가상 회로를 다시 설정했습니다. 더 이상 소켓을 사용할 수 없으므로 응용 프로그램이 소켓을 닫아야 합니다. UDP 데이터그램 소켓의 경우 이 오류는 이전 보내기 작업으로 인해 ICMP '포트 연결할 수 없음' 메시지가 발생했음을 나타냅니다.";
		case WSAEDISCON: // 10101
			return "소켓은 메시지 지향이며 가상 회로는 원격 쪽에서 정상적으로 닫혔습니다.";
		case WSAEFAULT: // 10014
			return "lpBuffers 매개 변수는 사용자 주소 공간의 유효한 부분에 완전히 포함되지 않습니다.";
		case WSAEINPROGRESS: // 10036
			return "차단 Windows Sockets 1.1 호출이 진행 중이거나 서비스 공급자가 여전히 콜백 함수를 처리하고 있습니다.";
		case WSAEINTR: // 10004
			return "WSACancelBlockingCall 함수에 의해(차단) 호출이 취소되었습니다.";
		case WSAEINVAL: // 10022
			return "소켓이 바인딩되지 않았습니다(예: 바인딩 사용).";
		case WSAEMSGSIZE: // 10040
			return "메시지가 너무 커서 지정된 버퍼에 맞지 않으며(신뢰할 수 없는 프로토콜에만 해당) 버퍼에 맞지 않는 메시지의 후행 부분이 삭제되었습니다.";
		case WSAENETDOWN: // 10050
			return "네트워크 하위 시스템이 실패했습니다.";
		case WSAENETRESET: // 10052
			return "연결 지향 소켓의 경우 이 오류는 작업이 진행되는 동안 오류를 감지한 연결 유지 작업으로 인해 연결이 끊어졌는지 나타냅니다. 데이터그램 소켓의 경우 이 오류는 TTL(Time to Live)이 만료되었음을 나타냅니다.";
		case WSAENOTCONN: // 10057
			return "소켓이 연결되지 않았습니다.";
		case WSAENOTSOCK: // 10038
			return "설명자가 소켓이 아닙니다.";
		case WSAEOPNOTSUPP: // 10045
			return "MSG_OOB 지정되었지만 소켓이 SOCK_STREAM 형식과 같은 스트림 스타일이 아니거나, OOB 데이터가 이 소켓과 연결된 통신 도메인에서 지원되지 않거나, 소켓이 단방향이며 보내기 작업만 지원합니다.";
		case WSAESHUTDOWN: // 10058
			return "소켓이 종료되었습니다. SD_RECEIVE 또는 SD_BOTH 설정된 방법을 사용하여 종료가 호출된 후에는 소켓에서 WSARecv를 호출할 수 없습니다.";
		case WSAETIMEDOUT: // 10060
			return "네트워크 오류로 인해 또는 피어 시스템이 응답하지 않기 때문에 연결이 삭제되었습니다.";
		case WSAEWOULDBLOCK: // 10035
			return "Windows NT: 겹치는 소켓: 겹치는 미해결 I/O 요청이 너무 많습니다. 오버랩되지 않은 소켓: 소켓이 차단 해제로 표시되고 수신 작업을 즉시 완료할 수 없습니다.";
		case WSANOTINITIALISED: // 10093
			return "이 함수를 사용하기 전에 성공적인 WSAStartup 호출이 발생해야 합니다.";
		case WSA_IO_PENDING: // 997
			return "겹치는 작업이 성공적으로 시작되었으며 나중에 완료가 표시됩니다.";
		case WSA_OPERATION_ABORTED: // 995
			return "소켓의 닫기로 인해 겹치는 작업이 취소되었습니다.";
		default:
			return "알 수 없는 WSA 수신 오류입니다.";
		}
	}


} // namespace Util

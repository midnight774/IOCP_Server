#pragma once

#include <winsock2.h>
#include <windows.h>
#include <mswsock.h>
#include <string>

#ifdef _WIN32
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "mswsock.lib")
#endif

class CEndpoint;

enum class SocketType
{
	Tcp,
	Udp,
};

static bool IsCalledStartUp = false;

// 소켓 클래스
class CSocket
{
public:
	CSocket();
	CSocket(SOCKET fd);
	CSocket(SocketType socketType);
	~CSocket();

public:
	static const int MaxReceiveLength = 8192;

	SOCKET m_SocketHandle;

	// AcceptEx 함수 포인터
	LPFN_ACCEPTEX AcceptEx = NULL;

	//AdressData TCP일때 사용
	char m_AcceptBuffer[1024] = {};
	sockaddr_in m_LocalAdress;
	sockaddr_in m_RemoteAdress;
	int			m_LocalAddrLength;
	int			m_RemoteAddrLength;

	//UDP 소켓일 때 사용 정보
	sockaddr	m_SenderAddr; //UDP 어디서 온건지

	//SendData
	WSAOVERLAPPED m_SendOverlappedStruct;
	int	m_BytesSent = - 1;
	int m_TotalBytesToSend = -1;
	int m_IsSendingOvelapped = false; //overlapped I/O Send 중이면 true입니다. 완료시 false

	
	//ReceiveData
	WSAOVERLAPPED m_ReceiveOverlappedStruct;

	char m_ReceiveBuffer[MaxReceiveLength];
	bool m_isReadOverlapped = false; //overlapped I/O Receive 중이면 true입니다. 완료시 false
	DWORD m_readFlags = 0;

public:
	void Bind(const CEndpoint& endpoint);
	void Connect(const CEndpoint& endpoint);
	int Send(const char* data, int length);
	int OverlappedSend(char* data, int length);
	int OverlappedSendTo(char* data, int Length, CEndpoint& Addr);
	void Close();
	void Listen();
	int Accept(CSocket& acceptedSocket, std::string& errorText);
	int StartUp();

	bool AcceptOverlapped(CSocket& acceptCandidateSocket, std::string& errorText);
	int UpdateAcceptContext(CSocket& listenSocket);

	CEndpoint GetPeerAddr();
	int Receive();

	int ReceiveOverlapped();
	int ReceiveFromOverlapped(CEndpoint& addr);

	void SetNonblocking();
	
};

std::string GetLastErrorAsString();

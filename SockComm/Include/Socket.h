#pragma once

#include <winsock2.h>
#include <windows.h>
#include <mswsock.h>

#include <string>

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
	static const int MaxReceiveLength = 8192;

	SOCKET m_SocketHandle;

	// AcceptEx 함수 포인터
	LPFN_ACCEPTEX AcceptEx = NULL;

	//AdressData
	char m_AcceptBuffer[1024] = {};
	sockaddr_in m_LocalAdress;
	sockaddr_in m_RemoteAdress;
	int			m_LocalAddrLength;
	int			m_RemoteAddrLength;

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

	CSocket();
	CSocket(SOCKET fd);
	CSocket(SocketType socketType);
	~CSocket();

	void Bind(const CEndpoint& endpoint);
	void Connect(const CEndpoint& endpoint);
	int Send(const char* data, int length);
	int OverlappedSend(char* data, int length);
	void Close();
	void Listen();
	int Accept(CSocket& acceptedSocket, std::string& errorText);
	int StartUp();

	bool AcceptOverlapped(CSocket& acceptCandidateSocket, std::string& errorText);
	int UpdateAcceptContext(CSocket& listenSocket);

	CEndpoint GetPeerAddr();
	int Receive();

	int ReceiveOverlapped();

	void SetNonblocking();
	
};

std::string GetLastErrorAsString();

#ifdef _WIN32
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "mswsock.lib")
#endif

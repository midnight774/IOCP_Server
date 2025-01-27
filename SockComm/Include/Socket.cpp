#include "stdafx.h"
#include <rpc.h>
#include <memory.h>
#include "Socket.h"
#include "Endpoint.h"
#include "Exception.h"

using namespace std;

std::string GetLastErrorAsString();

// 소켓 생성
CSocket::CSocket(SocketType socketType)
{
	if (!IsCalledStartUp)
	{
		StartUp();
		IsCalledStartUp = true;
	}

	if(socketType==SocketType::Tcp)
	{
		m_SocketHandle = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	}
	else 
	{
		m_SocketHandle = WSASocket(AF_INET, SOCK_DGRAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	}

	memset(&m_SendOverlappedStruct, 0, sizeof(m_SendOverlappedStruct));
	memset(&m_ReceiveOverlappedStruct, 0, sizeof(m_ReceiveOverlappedStruct));
}

CSocket::CSocket(SOCKET hSocket)
{
	if (!IsCalledStartUp)
	{
		StartUp();
		IsCalledStartUp = true;
	}

	m_SocketHandle = hSocket;

	memset(&m_SendOverlappedStruct, 0, sizeof(m_SendOverlappedStruct));
	memset(&m_ReceiveOverlappedStruct, 0, sizeof(m_ReceiveOverlappedStruct));
}

// 생성은 안한다.
CSocket::CSocket()
{
	static_assert(-1 == INVALID_SOCKET, "");

	m_SocketHandle = -1;

	memset(&m_SendOverlappedStruct, 0, sizeof(m_SendOverlappedStruct));
	memset(&m_ReceiveOverlappedStruct, 0, sizeof(m_ReceiveOverlappedStruct));
}

CSocket::~CSocket()
{
	Close();
}

void CSocket::Bind(const CEndpoint& endpoint)
{
	if (bind(m_SocketHandle, (sockaddr*)&endpoint.m_IPv4Endpoint, sizeof(endpoint.m_IPv4Endpoint)) < 0)
	{
		stringstream ss;
		ss << "bind failed:" << GetLastErrorAsString();
		throw Exception(ss.str().c_str());
	}
}

// endpoint가 가리키는 주소로 연결
void CSocket::Connect(const CEndpoint& endpoint)
{
	if (connect(m_SocketHandle, (sockaddr*)&endpoint.m_IPv4Endpoint, sizeof(endpoint.m_IPv4Endpoint)) < 0)
	{
		stringstream ss;
		ss << "connect failed:" << GetLastErrorAsString();
		throw Exception(ss.str().c_str());
	}
}

// 송신
int CSocket::Send(const char* data, int length)
{
	return send(m_SocketHandle, data, length, 0);
}

//Overlapped I/O 전용 송신 -> Windows 전용
int CSocket::OverlappedSend(char* data, int length)
{
	WSABUF Buffer;
	DWORD  BytesSent;
	Buffer.buf = reinterpret_cast<char*>(data);
	Buffer.len = strlen(data);
	
	return WSASend(m_SocketHandle, &Buffer, 1, &BytesSent, 0, &m_SendOverlappedStruct, NULL);
}

void CSocket::Close()
{
#ifdef _WIN32
	closesocket(m_SocketHandle);
#else
	close(m_SocketHandle);
#endif
}

void CSocket::Listen()
{
	listen(m_SocketHandle, 5000);
}

// 성공하면 0, 실패하면 다른 값을 리턴합니다.
int CSocket::Accept(CSocket& acceptedSocket, string& errorText)
{
	acceptedSocket.m_SocketHandle = accept(m_SocketHandle, NULL, 0);

	if (acceptedSocket.m_SocketHandle == -1)
	{
		errorText = GetLastErrorAsString();
		return -1;
	}
	else
		return 0;
}

//프로그램 실행 될 때 한 번은 무조건 호출되어야 함
int CSocket::StartUp()
{
	WSADATA wData;
	return WSAStartup(MAKEWORD(2, 2), &wData); // 2.2버전으로 Startup
}

bool CSocket::AcceptOverlapped(CSocket& acceptCandidateSocket, string& errorText)
{
	if (AcceptEx == nullptr)
	{
		DWORD bytes;
		// AcceptEx는 함수 포인터를 먼저 가져온 다음 호출할 수 있으므로 얻어오자
		WSAIoctl(m_SocketHandle,
			SIO_GET_EXTENSION_FUNCTION_POINTER,
			&UUID(WSAID_ACCEPTEX),
			sizeof(UUID),
			&AcceptEx,
			sizeof(AcceptEx),
			&bytes,
			nullptr,
			nullptr);

		if (AcceptEx == nullptr)
		{
			throw Exception("Getting AcceptEx ptr failed.");
		}
	}

	bool Ret = AcceptEx(m_SocketHandle,
		acceptCandidateSocket.m_SocketHandle,
		m_AcceptBuffer,
		0,
		50,
		50,
		nullptr,
		&m_ReceiveOverlappedStruct
	) == TRUE;
	
	return Ret;
}


// AcceptEX를 통해 얻어온 AcceptBuffer를 
// 구문 분석 하여 IP 주소 정보 뽑아낸다.
// 이 함수가 완료돼야 모든 연결 완료되는 것
int CSocket::UpdateAcceptContext(CSocket& listenSocket)
{
	GetAcceptExSockaddrs(m_AcceptBuffer,
		0,
		50,
		50,
		(sockaddr**)&m_LocalAdress,
		&m_LocalAddrLength,
		(sockaddr**)&m_RemoteAdress,
		&m_RemoteAddrLength);

	return setsockopt(m_SocketHandle, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
		(char*)&listenSocket.m_SocketHandle, sizeof(listenSocket.m_SocketHandle));
}


CEndpoint CSocket::GetPeerAddr()
{
	CEndpoint ret;
	socklen_t retLength = sizeof(ret.m_IPv4Endpoint);
	if (getpeername(m_SocketHandle, (sockaddr*)&ret.m_IPv4Endpoint, &retLength) < 0)
	{
		stringstream ss;
		ss << "getPeerAddr failed:" << GetLastErrorAsString();
		throw Exception(ss.str().c_str());
	}
	if(retLength > sizeof(ret.m_IPv4Endpoint))
	{
		stringstream ss;
		ss << "getPeerAddr buffer overrun: " << retLength;
		throw Exception(ss.str().c_str());
	}

	return ret;
}

int CSocket::Receive()
{
	return (int)recv(m_SocketHandle, m_ReceiveBuffer, MaxReceiveLength, 0);
}

//Overlapped I/O 수신
int CSocket::ReceiveOverlapped()
{
	WSABUF Buffer;
	Buffer.buf = m_ReceiveBuffer;
	Buffer.len = MaxReceiveLength;

	m_readFlags = { 0 };

	return WSARecv(m_SocketHandle, &Buffer, 1, NULL, &m_readFlags, &m_ReceiveOverlappedStruct, NULL);
}

// 논블록 소켓으로 모드를 설정합니다.
void CSocket::SetNonblocking()
{
	u_long val = 1;
	int ret = ioctlsocket(m_SocketHandle, FIONBIO, &val);

	if (ret != 0)
	{
		stringstream ss;
		ss << "bind failed:" << GetLastErrorAsString();
		throw Exception(ss.str().c_str());
	}
}

//Returns the last Win32 error, in string format. Returns an empty string if there is no error.
// 출처: https://stackoverflow.com/questions/1387064/how-to-get-the-error-message-from-the-error-code-returned-by-getlasterror
std::string GetLastErrorAsString()
{
#ifdef _WIN32
	//Get the error message, if any.
	DWORD errorMessageID = GetLastError();
	if (errorMessageID == 0)
		return std::string(); //No error message has been recorded

	LPSTR messageBuffer = nullptr;
	size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

	std::string message(messageBuffer, size);

	//Free the buffer.
	LocalFree(messageBuffer);

#else 
	std::string message = strerror(errno);
#endif
	return message;
}

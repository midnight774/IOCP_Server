#include "RemoteClient.h"

CRemoteClient::CRemoteClient()
{
}

CRemoteClient::CRemoteClient(SocketType socketType)
	:m_TcpSocket(socketType)
{
}

CRemoteClient::~CRemoteClient()
{
}

int CRemoteClient::OverlappedSend(char* data, int length)
{
	std::lock_guard<std::mutex> Lock(m_Mutex);
	return m_TcpSocket.OverlappedSend(data, length);
}


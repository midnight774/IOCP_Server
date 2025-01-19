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


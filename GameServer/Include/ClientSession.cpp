#include "ClientSession.h"

CClientSession::CClientSession()
{
}

CClientSession::CClientSession(SocketType socketType)	:
	m_TcpSocket(socketType)
{
}

CClientSession::~CClientSession()
{
}

int CClientSession::OverlappedSend(char* data, int length)
{
	return 0;
}

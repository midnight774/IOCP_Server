#include "ClientSession.h"
#include "GameObjectInfo.h"

CClientSession::CClientSession()
{
}

CClientSession::CClientSession(SocketType socketType)	:
	m_TcpSocket(socketType)
{
	m_ClientCharacterInfo = std::make_shared<CGameObjectInfo>();
}

CClientSession::~CClientSession()
{
}

int CClientSession::OverlappedSend(char* data, int length)
{
	return 0;
}

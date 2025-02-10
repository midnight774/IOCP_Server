#pragma once
#include "ServerShared.h"

class CGameObjectInfo;

class CClientSession
{
public:
	CClientSession();
	CClientSession(SocketType socketType);
	~CClientSession();

public:
	std::mutex					m_Mutex;
	std::shared_ptr<std::thread> m_Thread;
	CSocket m_TcpSocket;
	CEndpoint m_ClientUdpEndpoint;
	std::shared_ptr<CGameObjectInfo> m_ClientCharacterInfo;

public:
	//이동 데이터를 보낼 용도, 아이템 데이터 보낼 용도 등,
	int OverlappedSend(char* data, int length);

	void SetOverlappedReceiveFlag(bool Flag)
	{
		m_TcpSocket.m_isReadOverlapped = Flag;
	}
};


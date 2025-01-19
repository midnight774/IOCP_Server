#pragma once

#include "ServerShared.h"

class CRemoteClient
{
public:
	CRemoteClient();
	CRemoteClient(SocketType socketType);
	~CRemoteClient();

public:
	std::shared_ptr<std::thread> m_Thread;
	Socket m_TcpSocket;
};


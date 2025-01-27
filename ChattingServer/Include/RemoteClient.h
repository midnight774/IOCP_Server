#pragma once

#include "ServerShared.h"

class CRemoteClient
{
public:
	CRemoteClient();
	CRemoteClient(SocketType socketType);
	~CRemoteClient();

public:
	std::mutex					m_Mutex;
	std::shared_ptr<std::thread> m_Thread;
	Socket m_TcpSocket;


public:
	int OverlappedSend(char* data, int length);

	void SetOverlappedReadFlag(bool Flag)
	{
		m_TcpSocket.m_isReadOverlapped = Flag;
	}
};


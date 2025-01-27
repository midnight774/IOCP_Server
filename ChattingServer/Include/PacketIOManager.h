#pragma once
#include "ServerShared.h"

class CRemoteClient;

class CPacketIOManager
{
public:
	bool Init();

private:
	std::mutex m_IOMutex;
	Iocp m_Iocp;
	Socket m_ListenSocket;
	std::vector<std::shared_ptr<std::thread>> m_vecThreadPool;
	bool m_isStopWorking;
	std::shared_ptr<CRemoteClient> m_PendingClient;
	int m_ThreadCnt;

public:
	void IocpLoop();

private:
	void ProcessIocpEvent(const OVERLAPPED_ENTRY& Event);

private:
	void InitThreadPool(int ThreadCnt);
	const int GetLeastWorkThreadIdx();
	void EchoChattingData(char* EchoData, int DataLength);

	DECLARE_SINGLETON(CPacketIOManager);
};


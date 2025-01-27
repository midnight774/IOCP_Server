#pragma once
#include "ServerShared.h"

class CRemoteClient;

class CPacketIOManager
{
public:
	bool Init();

	volatile bool m_IsStopWorking = false;

private:
	std::mutex m_IOMutex;
	CIocp m_Iocp;
	CSocket m_ListenSocket;
	std::shared_ptr<CRemoteClient> m_PendingClient;

	std::vector<std::shared_ptr<std::thread>> m_vecThreadPool;
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


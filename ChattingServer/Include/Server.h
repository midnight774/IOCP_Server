#pragma once

#include "ServerShared.h"

class CRemoteClient;

class CServer
{
public:
	CServer();
	CServer(int ThreadCount);
	~CServer();

private:
	std::unordered_map<CRemoteClient*, std::shared_ptr<CRemoteClient>> m_mapRemoteClient;
	Iocp m_Iocp;
	Socket m_ListenSocket;
	std::shared_ptr<CRemoteClient> m_PendingClient;
	bool m_isStopWorking;
	int m_ThreadCnt;

	std::vector<std::shared_ptr<std::thread>> m_vecThreadPool;
	
public:
	bool InitServer();
	void CloseServer();

public:
	void EchoChattingData(const char* Data);

public:
	void InitThreadPool();
	const int GetLeastWorkThreadIdx();

public:
	int Run();

};


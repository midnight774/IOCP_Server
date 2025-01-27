#pragma once

#include "ServerShared.h"

class CServer
{
public:
	CServer();
	CServer(int ThreadCount);
	~CServer();

private:
	std::unordered_map<class CRemoteClient*, std::shared_ptr<CRemoteClient>> m_mapRemoteClient;
	Iocp m_Iocp;
	Socket m_ListenSocket;
	std::shared_ptr<CRemoteClient> m_RemoteClientCandidate;
	bool m_isStopWorking;
	
public:
	bool InitServer();
	void IocpLoop();
	void ProcessClientEnter(std::shared_ptr<CRemoteClient> RemoteClient);
	void ProcessClientLeave(std::shared_ptr<CRemoteClient> RemoteClient);
	void CloseServer();

public:
	int Run();

};


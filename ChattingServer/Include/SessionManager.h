#pragma once
#include "ServerShared.h"

class CRemoteClient;

class CSessionManager
{

public:
	bool Init();

private:
	
	std::mutex m_SessionMtx;
	std::unordered_map<CRemoteClient*, std::shared_ptr<CRemoteClient>> m_mapRemoteClient;

public:
	void InsertClient(std::shared_ptr<CRemoteClient> RemoteClient);
	void EraseClient(std::shared_ptr<CRemoteClient> RemoteClient);

	const std::unordered_map<CRemoteClient*, std::shared_ptr<CRemoteClient>>& GetAllSessions()
	{
		return m_mapRemoteClient;
	}

	std::shared_ptr<CRemoteClient> FindClient(CRemoteClient* KeyPtr);

	DECLARE_SINGLETON(CSessionManager);
};


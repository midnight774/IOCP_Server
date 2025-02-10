#pragma once
#include "ServerShared.h"

class CClientSession;

class CGameSessionManager
{
public:
	bool Init();

private:

	std::mutex m_SessionMtx;
	std::unordered_map<CClientSession*, std::shared_ptr<CClientSession>> m_mapClientSession;
	UINT m_AccClient;

public:
	void InsertClient(std::shared_ptr<CClientSession> RemoteClient);
	void EraseClient(std::shared_ptr<CClientSession> RemoteClient);

	std::shared_ptr<CClientSession> FindSessionByObjectID(UINT ObjID);

	const std::unordered_map<CClientSession*, std::shared_ptr<CClientSession>>& GetAllSessions()
	{
		return m_mapClientSession;
	}

	std::shared_ptr<CClientSession> FindClient(CClientSession* KeyPtr);

	const UINT GetNewClientID();

	DECLARE_SINGLETON(CGameSessionManager);
};


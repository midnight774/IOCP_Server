#include "GameSessionManager.h"
#include "ClientSession.h"
#include "GameObjectInfo.h"
#include "GamePacketManager.h"

DEFINITION_SINGLETON(CGameSessionManager);


CGameSessionManager::CGameSessionManager()
{
}

CGameSessionManager::~CGameSessionManager()
{
	while (m_mapClientSession.size() > 0)
	{
		auto iter = m_mapClientSession.begin();
		auto iterEnd = m_mapClientSession.end();

		iter->second->m_TcpSocket.Close();

		for (; iter != iterEnd;)
		{
			if (!iter->second->m_TcpSocket.m_isReadOverlapped)
			{
				iter = m_mapClientSession.erase(iter);
			}
			else
				iter++;
		}
	}
	
	m_mapClientSession.clear();
}

bool CGameSessionManager::Init()
{
	m_AccClient = 0;
	return true;
}

void CGameSessionManager::InsertClient(std::shared_ptr<CClientSession> ClientSession)
{
	std::lock_guard<std::mutex> Lock(m_SessionMtx);

	// I/O를 걸기
	{
		std::lock_guard<std::mutex> ClientLock(ClientSession->m_Mutex);
		ClientSession->m_TcpSocket.m_isReadOverlapped = true;
	}

	// 새 클라이언트 목록에 추가.
	ClientSession->m_ClientCharacterInfo->SetObjectID(m_AccClient);
	++m_AccClient;

	m_mapClientSession.insert({ ClientSession.get(), ClientSession });

	std::cout << "Client joined. There are " << m_mapClientSession.size() << " connections.\n";
}

void CGameSessionManager::EraseClient(std::shared_ptr<CClientSession> ClientSession)
{
	std::lock_guard<std::mutex> Lock(m_SessionMtx);

	ClientSession->m_TcpSocket.Close();
	m_mapClientSession.erase(ClientSession.get());

	std::cout << "Client left. There are " << m_mapClientSession.size() << " connections.\n";
}

std::shared_ptr<CClientSession> CGameSessionManager::FindSessionByObjectID(UINT ObjID)
{
	std::lock_guard<std::mutex> Lock(m_SessionMtx);
	auto iter = m_mapClientSession.begin();
	auto iterEnd = m_mapClientSession.end();

	for (; iter != iterEnd; ++iter)
	{
		std::shared_ptr<CGameObjectInfo> ObjInfo = iter->second->m_ClientCharacterInfo;
		if (ObjInfo->GetObjectID() == ObjID)
		{
			return iter->second;
		}
	}

	return nullptr;
}

std::shared_ptr<CClientSession> CGameSessionManager::FindClient(CClientSession* KeyPtr)
{
	std::lock_guard<std::mutex> Lock(m_SessionMtx);

	auto iter = m_mapClientSession.find(KeyPtr);
	if (iter != m_mapClientSession.end())
	{
		return iter->second;
	}

	return std::shared_ptr<CClientSession>(nullptr);
}

const UINT CGameSessionManager::GetNewClientID()
{
	return m_AccClient;
}

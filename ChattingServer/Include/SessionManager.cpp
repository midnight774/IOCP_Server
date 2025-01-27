#include "SessionManager.h"
#include "RemoteClient.h"

DEFINITION_SINGLETON(CSessionManager);


CSessionManager::CSessionManager()
{


}

CSessionManager::~CSessionManager()
{
	while (m_mapRemoteClient.size() > 0)
	{
		auto iter = m_mapRemoteClient.begin();
		auto iterEnd = m_mapRemoteClient.end();

		iter->second->m_TcpSocket.Close();

		for (; iter != iterEnd;)
		{
			if (!iter->second->m_TcpSocket.m_isReadOverlapped)
			{
				iter = m_mapRemoteClient.erase(iter);
			}
			else
				iter++;
		}
	}
}

bool CSessionManager::Init()
{
	return true;
}

void CSessionManager::InsertClient(std::shared_ptr<CRemoteClient> RemoteClient)
{
	std::lock_guard<std::mutex> Lock(m_SessionMtx);

	// I/O를 걸기
	{
		std::lock_guard<std::mutex> ClientLock(RemoteClient->m_Mutex);
		RemoteClient->m_TcpSocket.m_isReadOverlapped = true;
	}

	// 새 클라이언트 목록에 추가.
	m_mapRemoteClient.insert({ RemoteClient.get(), RemoteClient });

	std::cout << "Client joined. There are " << m_mapRemoteClient.size() << " connections.\n";
}

void CSessionManager::EraseClient(std::shared_ptr<CRemoteClient> RemoteClient)
{
	std::lock_guard<std::mutex> Lock(m_SessionMtx);

	RemoteClient->m_TcpSocket.Close();
	m_mapRemoteClient.erase(RemoteClient.get());

	std::cout << "Client left. There are " << m_mapRemoteClient.size() << " connections.\n";
}

std::shared_ptr<CRemoteClient> CSessionManager::FindClient(CRemoteClient* KeyPtr)
{
	std::lock_guard<std::mutex> Lock(m_SessionMtx);

	auto iter = m_mapRemoteClient.find(KeyPtr);
	if (iter != m_mapRemoteClient.end())
	{
		return iter->second;
	}

	return std::shared_ptr<CRemoteClient>(nullptr);
}

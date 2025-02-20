#pragma once
#include "ServerShared.h"

class CTaskWorker;
class CUpdateCharacterTask;
class CClientSession;

class CGameLogicManager
{
public:
	bool Init();

private:
	std::mutex m_Mtx;
	std::shared_ptr<std::thread>			m_GameLogicThread;
	//std::vector<std::shared_ptr<CTaskWorker>> m_vecGameLogicWorker;
	std::unordered_map<CClientSession*, std::shared_ptr<CClientSession>> m_mapUpdateClient;
	std::queue <std::shared_ptr<CClientSession>> m_qAttackingCharacter;
	bool m_IsStop;
	std::condition_variable					m_UpdateAvailable;

	LARGE_INTEGER m_Second;

private:
	void InitThreadPool(int ThreadCnt);
	const int GetLeastWorkThreadIdx();

public:
	void UpdateCharacter();
	void UpdateCharacterMove();
	void UpdateCharacterAttack();

	std::shared_ptr<CClientSession> FindClient(std::shared_ptr<CClientSession> pSession);
	void AddUpdateClient(std::shared_ptr<CClientSession> pSession);
	void RemoveUpdateClient(std::shared_ptr<CClientSession> pSession);
	void PushAttackingCharacter(std::shared_ptr<CClientSession> pSession);

	DECLARE_SINGLETON(CGameLogicManager);
};


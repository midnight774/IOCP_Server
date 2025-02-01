
#include "ServerShared.h"

class CClientSession;
class CTaskWorker;

class CGamePacketManager
{
public:
	bool Init();

	volatile bool m_IsStopWorking = false;

private:
	std::mutex m_IOMutex;
	CIocp m_Iocp;
	CSocket m_ListenSocket;
	CSocket m_UdpSocket;
	std::shared_ptr<CClientSession> m_PendingClient;

	std::vector<std::shared_ptr<CTaskWorker>> m_vecThreadWorker;
	int m_ThreadCnt;

public:
	void IocpLoop();

public:
	void ProcessIocpEvent(const OVERLAPPED_ENTRY& Event);

private:
	void InitThreadPool(int ThreadCnt);
	const int GetLeastWorkThreadIdx();
	void EchoGameData(char* EchoData, int DataLength);

	DECLARE_SINGLETON(CGamePacketManager);
};

#include "GameServer.h"
#include "GamePacketManager.h"
#include "GameSessionManager.h"
#include "GameLogicManager.h"

// TODO : TCP 스트림 일부만 송신하고 리턴하는 경우도 고려, 송수신 기능 패킷 매니저로 이관, 클라이언트 관리 기능 세션 매니저로 이관

volatile bool IsStop = false;
void CloseSignal(int Signal)
{
	if (Signal == SIGINT)
		IsStop = true;
}

CGameServer::CGameServer()
{
}

CGameServer::CGameServer(int ThreadCount)
{
}

CGameServer::~CGameServer()
{
}

bool CGameServer::InitServer()
{
	std::cout << "Game Server Starting...\n";

	if (!CGamePacketManager::GetInst()->Init())
		return false;

	if (!CGameSessionManager::GetInst()->Init())
		return false;

	if (!CGameLogicManager::GetInst()->Init())
		return false;
		
	std::cout << "Game Server ON\n";

	return true;
}


void CGameServer::CloseServer()
{
	std::cout << "Game Server Closing...\n";

	CGamePacketManager::GetInst()->DestroySingleInst();
	CGameSessionManager::GetInst()->DestroySingleInst();
	CGameLogicManager::GetInst()->DestroySingleInst();

	std::cout << "Game Server OFF.\n";
}

int CGameServer::Run()
{
	signal(SIGINT, CloseSignal);

	try
	{
		while (!IsStop)
		{
			CGamePacketManager::GetInst()->IocpLoop();
		}

		CloseServer();
	}
	catch (Exception& e)
	{
		std::cout << "Exception! " << e.what() << std::endl;
		return -1;
	}

	return 0;
}


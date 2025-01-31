
#include "ChattingServer.h"
#include "RemoteClient.h"
#include "PacketIOManager.h"
#include "SessionManager.h"

// TODO : TCP 스트림 일부만 송신하고 리턴하는 경우도 고려, 송수신 기능 패킷 매니저로 이관, 클라이언트 관리 기능 세션 매니저로 이관

volatile bool IsStop = false;
void CloseSignal(int Signal)
{
	if (Signal == SIGINT)
		IsStop = true;
}

CChattingServer::CChattingServer()
{
}

CChattingServer::CChattingServer(int ThreadCount)
{
}

CChattingServer::~CChattingServer()
{
}

bool CChattingServer::InitServer()
{
	std::cout << "Chatting Server Starting...\n";

	if (!CPacketIOManager::GetInst()->Init())
		return false;

	if (!CSessionManager::GetInst()->Init())
		return false;

	std::cout << "Chatting Server ON\n";

	return true;
}


void CChattingServer::CloseServer()
{
	std::cout << "Chatting Server Closing...\n";

	CPacketIOManager::GetInst()->DestroySingleInst();
	CSessionManager::GetInst()->DestroySingleInst();

	std::cout << "Chatting Server OFF.\n";
}

int CChattingServer::Run()
{
	signal(SIGINT, CloseSignal);

	try
	{
		while(!IsStop)
		{
			CPacketIOManager::GetInst()->IocpLoop();
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


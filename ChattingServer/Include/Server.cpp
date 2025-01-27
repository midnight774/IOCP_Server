
#include "Server.h"
#include "RemoteClient.h"
#include "PacketIOManager.h"
#include "SessionManager.h"

// TODO : TCP 스트림 일부만 송신하고 리턴하는 경우도 고려, 송수신 기능 패킷 매니저로 이관, 클라이언트 관리 기능 세션 매니저로 이관

CServer::CServer() :	m_isStopWorking(false),
						m_ListenSocket(SocketType::Tcp)
{
	m_ListenSocket.Bind(Endpoint("0.0.0.0", 5555));
	
}

CServer::CServer(int ThreadCount) : m_isStopWorking(false),
									m_ListenSocket(SocketType::Tcp),
									m_ThreadCnt(ThreadCount),
									m_Iocp(ThreadCount)
{
}

CServer::~CServer()
{
}

bool CServer::InitServer()
{
	std::cout << "Chatting Server Starting...\n";

	if (!CPacketIOManager::GetInst()->Init())
		return false;

	if (!CSessionManager::GetInst()->Init())
		return false;

	std::cout << "Chatting Server ON\n";

	return true;
}


void CServer::CloseServer()
{
	std::cout << "Chatting Server Closing...\n";

	CPacketIOManager::GetInst()->DestroySingleInst();
	CSessionManager::GetInst()->DestroySingleInst();

	std::cout << "Chatting Server OFF.\n";
}

void CServer::EchoChattingData(const char* Data)
{
}

void CServer::InitThreadPool()
{
	
}

const int CServer::GetLeastWorkThreadIdx()
{
	return 0;
}

int CServer::Run()
{
	try
	{
		CPacketIOManager::GetInst()->IocpLoop();
		CloseServer();
	}
	catch (Exception& e)
	{
		std::cout << "Exception! " << e.what() << std::endl;
		return -1;
	}

	return 0;
}

